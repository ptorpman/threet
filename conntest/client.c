/* Copyright (c) 2012  Peter R. Torpman (peter at torpman dot se) 
 
   This file is part of Torpman's Test Tools  
   https://github.com/ptorpman/threet

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
#include "client.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "conntest.h"
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include "common.h"
#include "debug.h"

/*============================================================================*/
/* FUNCTION DECLARATIONS                                                      */
/*============================================================================*/
static void* send_thread(void* data);
static void* conn_thread(void* data);
static void* tp_thread(void* data);

/* Returns socket that was connected */
static int  connect_to_server(char* serverIp, uint32_t serverPort);

/*============================================================================*/
/* VARIABLES                                                                  */
/*============================================================================*/
static client_data_t* gClients[0xFFFF];
static pthread_mutex_t gMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t gMutexSent = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t gMutexRate = PTHREAD_MUTEX_INITIALIZER;
static uint32_t gNumConn = 0;
static int gDoExit = 0;
static uint64_t gNumSentSinceStart = 0;
static uint64_t gNumConnSinceStart = 0;
static uint64_t gNumSentSinceMeasure = 0;
static pthread_t gTpThread;
static uint8_t gSendBuffer[0xFFFFFF];
static int64_t gBytesToSend = 100;
static int gClientId = 0;

static char gServerIp[0xFF];
static uint32_t gServerPort = 0;

/*============================================================================*/
/* FUNCTION DEFINITIONS                                                       */
/*============================================================================*/

void
client_start(char* serverIp, uint32_t serverPort,
             uint32_t numConn, uint32_t wantedTp, int client_id)
{
   int res = 0;

   gClientId = client_id;
   
   strcpy(gServerIp, serverIp);
   gServerPort = serverPort;
   
   memset(gSendBuffer, 0xFF, 0xFFFFFF);
   
   gNumConn = numConn;
   gWantedTp = wantedTp;
   
   for (int i = 0; i < (int) numConn; i++) {
      gClients[i] = (client_data_t*) malloc(sizeof(client_data_t));
   }
   
   if (gWantedTp > 0) {
      /* First, connect all clients to the server */
      for (int i = 0; i < (int) numConn; i++) {
         client_data_t* cliData = gClients[i];

         memset(cliData, 0, sizeof(client_data_t));
      
         cliData->clientNum = i;
      
         cliData->sock = connect_to_server(serverIp, serverPort);
      
         if (cliData->sock < 0) {
            debug_print( "ERROR: Could not open socket (%d)!\n", i);
            return;
         }    
      }
   }

   /* Lock mutex until all are created... */
   pthread_mutex_lock(&gMutex);
   
   for (int i = 0; i < (int) numConn; i++) {
      pthread_t t;
      pthread_attr_t attr;
      pthread_attr_init(&attr);
      pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
      pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
      
      if (gWantedTp > 0) {
         res = pthread_create(&gClients[i]->thread, &attr, send_thread,
                              (void*) gClients[i]);
      }
      else {
         /* Connection establishment thread... */
         res = pthread_create(&gClients[i]->thread, &attr, conn_thread,
                              (void*) gClients[i]);
      }

      if (res != 0) {
         debug_print( "* Failed to create sender #%d\n",
                 gClients[i]->clientNum);
         
         perror("Thread error!");
         gDoExit = 1;
         goto outro;
      }
   }

   /* Start througput display thread */
   res = pthread_create(&gTpThread, NULL, tp_thread, NULL);
   
   if (res < 0) {
      debug_print( "ERROR: Failed to start throughput thread!\n");
      return;
   }

   pthread_mutex_unlock(&gMutex);

   if (get_aggregator_port() != -1) {
	   aggregator_register(gClientId, "127.0.0.1");
   }
   
   debug_print( "Clients running (conns=%u)...(tp=%u kbps)\n",
				numConn, gWantedTp);

   while (1) {
      usleep(1000);

      if (gDoExit) {
         break;
      }
   }
   
 outro:
   /* Wait for all threads to finish */
   for (int i = 0; i < (int) numConn; i++) {
      pthread_join(gClients[i]->thread, NULL);
   }
}

static void*
send_thread(void* data)
{
   client_data_t* cliData = (client_data_t*) data;
   int res = 0;
   
   //debug_print( "* Sender #%d started\n", cliData->clientNum);

   /* When we get the lock we start sending... */
   pthread_mutex_lock(&gMutex);

   if (gDoExit) {
      pthread_exit(cliData);
   }
   
   pthread_mutex_unlock(&gMutex);

   int loops = 0;


   while (1) {
      usleep(10000);

      if (gDoExit) {
         close(cliData->sock);
         pthread_exit(cliData);
      }

      res = send(cliData->sock, gSendBuffer, gBytesToSend, 0);

      if (res < 0) {
         debug_print( "* Client #%d failed to send\n", cliData->clientNum);
      }
      else {
         pthread_mutex_lock(&gMutexSent);
         gNumSentSinceMeasure += res;
         gNumSentSinceStart += res;
         pthread_mutex_unlock(&gMutexSent);
      }
   }
   
   pthread_exit(cliData);
}

static void*
conn_thread(void* data)
{
   client_data_t* cliData = (client_data_t*) data;
   int res = 0;

   /* When we get the lock we start connecting... */
   pthread_mutex_lock(&gMutex);

   if (gDoExit) {
      pthread_exit(cliData);
   }
   
   pthread_mutex_unlock(&gMutex);
   
   while (1) {
      res = connect_to_server(gServerIp, gServerPort);

      if (res < 0) {
         debug_print( "ERROR: Failed to connect to server %s:%u!\n",
                 gServerIp, gServerPort);
         pthread_exit(cliData);
      }

      pthread_mutex_lock(&gMutexSent);
      gNumConnSinceStart++;
      pthread_mutex_unlock(&gMutexSent);

      /* Just close it! */
      close(res);
   }

   pthread_exit(cliData);
}


void
client_exit(int sig)
{
   /* Mark this flag to make the threads end themselves... */
   gDoExit = 1;
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

#ifdef DEBUG
   char tmpStr[0xFFF];
#endif
   
   while (1) {
      /* Sleep 1 second */
      gettimeofday(&tLoopStart, NULL);
      usleep(1000000);

      if (gDoExit) {
         pthread_exit(NULL);
      }

      numSecs++;
      pthread_mutex_lock(&gMutexSent);
      numBytes             = gNumSentSinceMeasure;
      numBytesSinceStart   = gNumSentSinceStart;
      numConnSinceStart    = gNumConnSinceStart;
      gNumSentSinceMeasure = 0; /* Reset for next calculation */
      pthread_mutex_unlock(&gMutexSent);

      gettimeofday(&tNow, NULL);

      /* In seconds since start of traffic */
      elapsed = (tNow.tv_sec - tLoopStart.tv_sec) +
         ((tNow.tv_usec - tLoopStart.tv_usec) / 1000000);
      
      
      /* bits per second */
      tPutNow = (numBytes * 8) / elapsed;

	  if (get_aggregator_port() != -1) {
		  aggregator_report(gClientId, tPutNow / 1024, "127.0.0.1");
	  }

	  /* No aggregator used. Print output */
	  
      tPutAverage = (numBytesSinceStart * 8 / 1024) / numSecs;
      tConnAverage = (double) (numConnSinceStart / numSecs);
      
      sprintf(outPutStr,
               "Throughput @%3u s: %10.2f kbps %10.2f Mbps (Avg: %10.2f kbps %10.2f "
			  "conn/s)\n",
              numSecs,
              (tPutNow / 1024), (tPutNow / (1024 * 1024)), tPutAverage / 1024, tConnAverage);
              

      double diff;
      
      if (gWantedTp > 0 && (tPutNow / 1024) > gWantedTp) {
         diff = (tPutNow / 1024) - gWantedTp; /* in kbps */

         /* Allow 1 percent diff from wanted throughput */
         if (diff > (0.01 * gWantedTp)) {
                   
            
            /* Throughput is too high. Try to decrease it */
            pthread_mutex_lock(&gMutexRate);
            
            gBytesToSend = gBytesToSend - ((0.25 * diff) / gNumConn);

            if (gBytesToSend < 0) {
               gBytesToSend = 1;
            }
            
#ifdef DEBUG
            sprintf(tmpStr, "               Diff: %10.2f kbps. Decreasing to %d bytes\n",
                    diff, (int) gBytesToSend);
            strcat(outPutStr, tmpStr);
#endif            
            pthread_mutex_unlock(&gMutexRate);

         }
      }
      else if (gWantedTp > 0 && (tPutNow / 1024) < gWantedTp) {
         diff = gWantedTp - (tPutNow / 1024); /* in kbps */

         /* Allow 1 percent diff from wanted throughput */
         if (diff > (0.01 * gWantedTp)) {
            /* Throughput is too low. Try to increase it */
            pthread_mutex_lock(&gMutexRate);
            gBytesToSend = gBytesToSend + ((0.25 * diff) / gNumConn);

#ifdef DEBUG
            sprintf(tmpStr, "               Diff: %10.2f kbps. Increasing to %d bytes\n",
                    diff, (int) gBytesToSend);
            strcat(outPutStr, tmpStr);
#endif

            pthread_mutex_unlock(&gMutexRate);
         }
      }

	  if (get_aggregator_port() == -1) {
		  debug_print( "%s", outPutStr);
	  }
			  
      
   }

   pthread_exit(NULL);
}



static int
connect_to_server(char* serverIp, uint32_t serverPort)
{
   /* Get server address */
   struct hostent* he;
   struct in_addr  inAddr;
   int             sock  = -1;
   int             reuse = 1;
   int             res   = 0;
   struct sockaddr_in sinAddr;

   memset(&sinAddr, 0, sizeof(struct sockaddr_in));
   
   /* A host name? */
   he = gethostbyname(serverIp);

   if (!he) {
      /* Nope. Check dotted IP... */
      if (inet_aton(serverIp, &inAddr) == 0) {
         debug_print( "ERROR: Unknown server address!\n");
         return -1;
      }
   }
   else {
      inAddr = *((struct in_addr*) he->h_addr);
   }

   sock = socket(AF_INET, SOCK_STREAM, 0);

   if (sock < 0) {
      debug_print( "ERROR: Failed to open socket!\n");
      return -1;
   }

   /* Set reusable socket */
   setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*) &reuse, sizeof(reuse));
   
   sinAddr.sin_family = AF_INET;
   sinAddr.sin_addr = inAddr;
   sinAddr.sin_port = htons(serverPort);

   res = connect(sock, (struct sockaddr*) &sinAddr, sizeof(struct sockaddr));
   
   if (res < 0) {
      debug_print( "ERROR: Failed to connect socket! errno=%d\n", errno);
      perror("Could not connect socket");
      return -1;
   }

   return sock;
}

