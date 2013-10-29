/* Copyright (c) 2012  Peter R. Torpman (peter at torpman dot se) 
 
   This file is part of Torpman's Test Tools  
   http://sourceforge.net/projects/torptest) 

   Torpman's Test Tools is free software; you can redistribute it and/or modify 
   it under the terms of the GNU General Public License as published by 
   the Free Software Foundation; either version 3 of the License, or 
   (at your option) any later version. 

   Torpman's Test Tools is distributed in the hope that it will  
   be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of 
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
   GNU General Public License for more details. 

   You should have received a copy of the GNU General Public License 
   along with this program.  If not, see <http://www.gnu.or/licenses/>. 
*/

/*============================================================================*/
/* INCLUDES                                                                   */
/*============================================================================*/
#include "server.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <poll.h>
#include <unistd.h>
#include <sys/time.h>

#define _REENTRANT
#include <pthread.h>

#include "conntest.h"
#include "common.h"
#include "debug.h"

/*============================================================================*/
/* FUNCTION DECLARATIONS                                                      */
/*============================================================================*/
static void eternal_loop(void);
static void* recv_thread(void* data);
static void* tp_thread(void* data);

/*============================================================================*/
/* VARIABLES                                                                  */
/*============================================================================*/
static int gSocket = -1;
static uint32_t gNumConn = -1;
static uint32_t gNumClosed = 0;
static pthread_t gTpThread;

static pthread_mutex_t gMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t gMutexClosed = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t gMutexRecv = PTHREAD_MUTEX_INITIALIZER;
static uint64_t gRecvBytes = 0;
static uint64_t gRecvBytesSinceStart = 0;
static uint64_t gNumConnSinceStart = 0;
static int gClientId = 0;

/*============================================================================*/
/* FUNCTION DEFINITIONS                                                       */
/*============================================================================*/

void
server_start(uint32_t port, uint32_t numConn, int proc_id)
{
   int res = -1;


   gClientId = proc_id;
   gNumConn = numConn;
   
   /* Open up a server socket for incoming connections */

   gSocket = socket(AF_INET, SOCK_STREAM, 0);

   if (gSocket < 0) {
      inform_user("ERROR: Failed to open socket!\n");
      return;
   }

   /* Set reusable socket */
   int reuse = 1;
   setsockopt(gSocket, SOL_SOCKET, SO_REUSEADDR, (char*) &reuse, sizeof(reuse));

   
   struct sockaddr_in servAddr;
   memset(&servAddr, 0, sizeof(struct sockaddr_in));

   servAddr.sin_family = AF_INET;
   servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
   servAddr.sin_port = htons(port);
   
   res = bind(gSocket, (struct sockaddr*) &servAddr, sizeof(servAddr));
   
   if (res < 0) {
      inform_user("ERROR: Failed to bind socket!\n");
      return;
   }

   res = listen(gSocket, gNumConn);

   if (res < 0) {
      inform_user("ERROR: Failed to listen on socket!\n");
      return;
   }

   inform_user("* Server socket bound to port %u\n", port);

   /* Enter eternal loop */
   eternal_loop();
}


static void
eternal_loop(void)
{
   /* Start polling the server socket and accept incoming connections. */

   struct pollfd fd[1];
   int res = 0;
   int numConnected = 0;

   fd[0].fd = gSocket;
   fd[0].events = POLLIN;

   /* Start througput display thread */
   res = pthread_create(&gTpThread, NULL, tp_thread, NULL);
   
   if (res < 0) {
      inform_user("ERROR: Failed to start througput thread!\n");
      return;
   }

   if (get_aggregator_port() != -1) {
	   aggregator_register(gClientId, "127.0.0.1");
   }
   
   while (1) {
      res = poll(fd, 1, -1);

      if (res < 0) {
         inform_user("ERROR: Failed to poll socket!\n");
         return;
      }

      if (res > 0 && fd[0].revents == POLLIN) {
         /* Accept incoming connections */
         struct sockaddr_in cliAddr;
         uint32_t cliLen = sizeof(struct sockaddr_in);
         
         res = accept(fd[0].fd, (struct sockaddr*) &cliAddr, &cliLen);
         
         if (res < 0) {
            inform_user("ERROR: Failed to accept connection!\n");
            return;
         }

         pthread_mutex_lock(&gMutexRecv);
         gNumConnSinceStart++;
         pthread_mutex_unlock(&gMutexRecv);
         
         if (gWantedTp == 0) {
            close(res);
            continue;
         }
         
         client_data_t* cliData =
            (client_data_t*) malloc(sizeof(client_data_t));

         memcpy(&cliData->sinAddr, &cliAddr, sizeof(struct sockaddr_in));

         cliData->sock      = res;
         cliData->clientNum = numConnected;
         

         if (gWantedTp != 0) {
            /* Start client thread */
            res = pthread_create(&cliData->thread, NULL,
                                 recv_thread, (void*) cliData);
            
            if (res < 0) {
               inform_user("ERROR: Failed to start client thread!\n");
               return;
            }
         }

         numConnected++;
      }

      if (gNumClosed == gNumConn) {
         inform_user("* All clients closed!\n");
         return;
      }
   }
}

static void*
recv_thread(void* data)
{
   client_data_t* cliData = (client_data_t*) data;

   inform_user("* Recv from client %d\n", cliData->clientNum);

   int    res     = 0;
   char   buff[0xFFFF];
   size_t buffLen = 0xFFFF;
   
   /* Enter loop to handle data from client */
   while (1) {
      res = recv(cliData->sock, buff, buffLen, 0);

      if (res == 0) {
         /* Remote side has closed down... */
         pthread_mutex_lock(&gMutex);
         gNumClosed++;
         pthread_mutex_unlock(&gMutex);
         pthread_exit(cliData);
      }

      /* Update number of received bytes */
      pthread_mutex_lock(&gMutexRecv);
      gRecvBytes += res;
      gRecvBytesSinceStart += res;
      pthread_mutex_unlock(&gMutexRecv);
   }
   
   pthread_exit(cliData);
}


static void*
tp_thread(void* data)
{
   /* This thread is used for displaying the current throughput. */
   uint32_t numSecs = 0;
   uint64_t numBytes = 0;
   uint64_t numBytesSinceStart = 0;
   uint64_t numConnSinceStart = 0;
   struct timeval tLoopStart;
   struct timeval tNow;
   uint32_t elapsed = 0;
   double tPutNow;
   double tPutAverage = 0;
   double tConnAverage = 0;
   char outPutStr[0xFFF];

   while (1) {
      /* Sleep 1 second */
      gettimeofday(&tLoopStart, NULL);
      usleep(1000000);

      numSecs++;
      pthread_mutex_lock(&gMutexRecv);
      numBytes           = gRecvBytes;
      numBytesSinceStart = gRecvBytesSinceStart;
      numConnSinceStart  = gNumConnSinceStart;
      gRecvBytes         = 0;   /* Reset for next calculation */
      pthread_mutex_unlock(&gMutexRecv);

      gettimeofday(&tNow, NULL);

      /* In seconds since start of traffic */
      elapsed = (tNow.tv_sec - tLoopStart.tv_sec) +
         ((tNow.tv_usec - tLoopStart.tv_usec) / 1000000);
      
      
      /* bits per second */
      tPutNow = (numBytes * 8) / elapsed;

      tPutAverage = (numBytesSinceStart * 8 / 1024) / numSecs;
      tConnAverage = (double) (numConnSinceStart / numSecs);

	  if (get_aggregator_port() == -1) {
		  debug_print("Throughput @%3u s: %10.2f kbps %10.2f Mbps (Avg: %10.2f "
					  "kbps %10.2f conn/s)\n",
					  numSecs,
					  (tPutNow / 1024), (tPutNow / (1024 * 1024)), tPutAverage,
					  tConnAverage);
	  }
	  else {
		  aggregator_report(gClientId, tPutNow / 1024, "127.0.0.1");
	  }
   }

   pthread_exit(NULL);
}
