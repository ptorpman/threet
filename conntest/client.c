/* Copyright (c) 2012  Peter Torpman (peter at torpman dot se) 
 
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
static void *send_thread(void *data);
static void *conn_thread(void *data);
static void *tp_thread(void *data);

/* Returns socket that was connected */
static int connect_to_server(char *serverIp, uint32_t serverPort);
static void adjust_traffic(double tPutNow);
static void allocate_clients(uint32_t num);
static int connect_clients(uint32_t numConn);
static int create_client_threads(uint32_t num);

/*============================================================================*/
/* VARIABLES                                                                  */
/*============================================================================*/
static client_data_t *gClients[0xFFFF];
static pthread_mutex_t gMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t gMutexRate = PTHREAD_MUTEX_INITIALIZER;
static uint32_t gNumConn = 0;
static int gDoExit = 0;
static uint64_t gNumConnSinceStart = 0;
static pthread_t gTpThread;
static uint8_t gSendBuffer[0xFFFFFF];
static int64_t gBytesToSend = 100;
static uint32_t gPacketSize = 0;
static int gClientId = 0;
static char gServerIp[0xFF];
static uint32_t gServerPort = 0;

/*============================================================================*/
/* LOCAL FUNCTION DEFINITIONS                                                 */
/*============================================================================*/

static void
allocate_clients(uint32_t num)
{
    for (int i = 0; i < (int) num; i++) {
        gClients[i] = (client_data_t *) malloc(sizeof (client_data_t));
        pthread_mutex_init(&gClients[i]->mutex, NULL); 
    }
}

static int
connect_clients(uint32_t num)
{
    for (int i = 0; i < (int) num; i++) {
        client_data_t *cliData = gClients[i];
        memset(cliData, 0, sizeof (client_data_t));
        cliData->clientNum = i;
        cliData->sock = connect_to_server(gServerIp, gServerPort);
        if (cliData->sock < 0) {
            debug_print("ERROR: Could not open socket (%d)!\n", i);
            return 1;
        }
    }

    return 0;
}

static int
create_client_threads(uint32_t num)
{
    int res = 0;

    pthread_mutex_lock (&gMutex);

    /* Start througput display thread */
    res = pthread_create(&gTpThread, NULL, tp_thread, NULL);
    if (res < 0) {
        debug_print("ERROR: Failed to start throughput thread!\n");
        return 1;
    }
    
    for (int i = 0; i < (int) num; i++) {
        pthread_t t;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
        
        if (gWantedTp > 0 || gPacketSize > 0) {
            res = pthread_create(&gClients[i]->thread, &attr,
                                 send_thread, (void *) gClients[i]);
        }

        else {
            /* Connection establishment thread... */
            res = pthread_create(&gClients[i]->thread, &attr,
                                 conn_thread, (void *) gClients[i]);
        }

        if (res != 0) {
            debug_print("* Failed to create sender #%d\n", gClients[i]->clientNum);
            perror("Thread error!");
            gDoExit = 1;
            goto outro;
        }
    }

    pthread_mutex_unlock(&gMutex);

    return 0;
    
  outro:
    /*
     * Wait for all threads to finish 
     */
    for (int i = 0; i < (int) num; i++) {
        pthread_join(gClients[i]->thread, NULL);
    }

    return 1;
}

static void *
send_thread(void *data)
{
    client_data_t *cliData = (client_data_t *) data;
    int res = 0;

    pthread_mutex_lock(&gMutex);
    if (gDoExit) {
        pthread_exit(cliData);
    }
    pthread_mutex_unlock(&gMutex);
   
    uint32_t bytesToSend = gPacketSize > 0 ? gPacketSize : gBytesToSend;

    while (1) {
        usleep(10);
        
        if (gDoExit) {
            debug_print("Client exit\n");
            close(cliData->sock);
            pthread_exit(cliData);
        }

        res = send(cliData->sock, gSendBuffer, bytesToSend, MSG_NOSIGNAL);

        if (res > 0) {
            cliData->numSentSinceStart += res;
            cliData->numSentSinceMeasure += res;
        }
    }
    
    pthread_exit(cliData);
}

static void *
conn_thread(void *data)
{
    client_data_t *cliData = (client_data_t *) data;
    int res = 0;

    /*
     * When we get the lock we start connecting... 
     */
    pthread_mutex_lock(&gMutex);
    if (gDoExit) {
        pthread_exit(cliData);
    }
    pthread_mutex_unlock(&gMutex);
    while (1) {
        res = connect_to_server(gServerIp, gServerPort);
        if (res < 0) {
            debug_print("ERROR: Failed to connect to server %s:%u!\n", gServerIp, gServerPort);
            pthread_exit(cliData);
        }
        pthread_mutex_lock(&cliData->mutex);
        cliData->numConnSinceStart++;
        pthread_mutex_unlock(&cliData->mutex);

        /*
         * Just close it! 
         */
        close(res);
    }
    pthread_exit(cliData);
}

static void *
tp_thread(void *data)
{
    /*
     * This thread is used for displaying the current throughput. 
     */
    uint64_t numBytes = 0;
    uint64_t numBytesSinceStart = 0;
    struct timeval startTimeLoop;
    uint32_t elapsed = 0;
    double bitsPerSecNow;
    double bitsPerSecAverage = 0;
    struct timeval startTime;

    gettimeofday(&startTime, NULL);
    
    while (1) {

        /* Sleep 1 second */
        gettimeofday(&startTimeLoop, NULL);
        usleep(1000000);
        if (gDoExit) {
            break;
        }

        numBytes = 0;
        numBytesSinceStart = 0;
        
        for (uint32_t i = 0; i < gNumConn; i++) {
            pthread_mutex_lock(&gClients[i]->mutex);
            numBytes += gClients[i]->numSentSinceMeasure;
            gClients[i]->numSentSinceMeasure = 0;
            numBytesSinceStart += gClients[i]->numSentSinceStart;
            pthread_mutex_unlock(&gClients[i]->mutex);
        }

	    struct timeval timeNow;
        gettimeofday(&timeNow, NULL);
        uint32_t numSecs = (timeNow.tv_sec - startTime.tv_sec) + ((timeNow.tv_usec - startTime.tv_usec) / 1000000);

        /* In seconds since start of traffic */
        elapsed = (timeNow.tv_sec - startTimeLoop.tv_sec) + ((timeNow.tv_usec - startTimeLoop.tv_usec) / 1000000);

        bitsPerSecNow = (numBytes * 8) / elapsed;
        
        if (get_aggregator_port () != -1) {
            aggregator_report(gClientId, bitsPerSecNow / 1024, "127.0.0.1");
        }
		else {
			inform_user("Throughput @%3u s: %10.2f kbps %10.2f Mbps (Avg: %10.2f Mbps)\n",
                    	numSecs, (bitsPerSecNow / 1024),
                    	(bitsPerSecNow / (1024 * 1024)), bitsPerSecAverage / 1024);
		}

        /* No aggregator used. Print output */
        bitsPerSecAverage = (numBytesSinceStart * 8 / 1024) / numSecs;

        if (gPacketSize == 0) {
            adjust_traffic(bitsPerSecNow);
        }
    }

    pthread_exit(NULL);
}

static void
adjust_traffic(double tPutNow)
{
    double diff;

    if (gWantedTp > 0 && (tPutNow / 1024) > gWantedTp) {
        diff = (tPutNow / 1024) - gWantedTp;  /* in kbps */

        /*
         * Allow 1 percent diff from wanted throughput 
         */
        if (diff > (0.01 * gWantedTp)) {

            /*
             * Throughput is too high. Try to decrease it 
             */
            pthread_mutex_lock(&gMutexRate);
            gBytesToSend = gBytesToSend - ((0.25 * diff) / gNumConn);
            if (gBytesToSend < 0) {
                gBytesToSend = 1;
            }
            pthread_mutex_unlock(&gMutexRate);
        }
    }

    else if (gWantedTp > 0 && (tPutNow / 1024) < gWantedTp) {
        diff = gWantedTp - (tPutNow / 1024);  /* in kbps */

        /*
         * Allow 1 percent diff from wanted throughput 
         */
        if (diff > (0.01 * gWantedTp)) {

            /*
             * Throughput is too low. Try to increase it 
             */
            pthread_mutex_lock(&gMutexRate);
            gBytesToSend = gBytesToSend + ((0.25 * diff) / gNumConn);
            pthread_mutex_unlock(&gMutexRate);
        }
    }
}

static int
connect_to_server(char *serverIp, uint32_t serverPort)
{
    /*
     * Get server address 
     */
    struct hostent *he;
    struct in_addr inAddr;
    int sock = -1;
    int reuse = 1;
    int res = 0;
    struct sockaddr_in sinAddr;
    memset(&sinAddr, 0, sizeof (struct sockaddr_in));

    /*
     * Check dotted IP... 
     */
    if (inet_aton(serverIp, &inAddr) == 0) {
        debug_print("ERROR: Unknown server address!\n");
        return -1;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
        debug_print("ERROR: Failed to open socket!\n");
        return -1;
    }

    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse, sizeof (reuse));

    sinAddr.sin_family = AF_INET;
    sinAddr.sin_addr = inAddr;
    sinAddr.sin_port = htons(serverPort);

    res = connect(sock, (struct sockaddr *) &sinAddr, sizeof (struct sockaddr));
    if (res < 0) {
        debug_print("ERROR: Failed to connect socket! errno=%d\n", errno);
        perror("Could not connect socket");
        return -1;
    }
    return sock;
}

/*============================================================================*/
/* GLOBAL FUNCTION DEFINITIONS                                                */
/*============================================================================*/

void
client_start(char *serverIp,
             uint32_t serverPort,
             uint32_t numConn,
             uint32_t wantedTp,
             uint32_t packetSize,
             int client_id)
{
    int res = 0;
    gClientId = client_id;
    strcpy(gServerIp, serverIp);
    
    gServerPort = serverPort;
    memset(gSendBuffer, 0xFF, 0xFFFFFF);
    gNumConn = numConn;
    gWantedTp = wantedTp;
    gPacketSize = packetSize;

    if (packetSize > 0) {
        inform_user("* Will send packets of size: %d\n", packetSize);
        gWantedTp = 0;
        gPacketSize = packetSize - ETH_HEADER_SIZE;
    }
    else {
        inform_user("* Will aim for throughput: %d\n", gWantedTp);
    }

    allocate_clients(numConn);
    
    if (connect_clients(numConn) != 0)
    {
        inform_user("* Failed to connect clients\n");
        return;
    }

    if (create_client_threads(numConn) != 0)
    {
        inform_user("* Failed to create client threads\n");
        return;
    }
      
    if (get_aggregator_port() != -1) {
        aggregator_register(gClientId, "127.0.0.1");
    }

    debug_print("Clients running (conns=%u)...(tp=%u kbps pktsize=%u)\n", numConn, gWantedTp, gPacketSize);

    while (1) {
        usleep(1000);
        if (gDoExit) {
            break;
        }
    }
}


void
client_exit(int sig)
{
    /*
     * Mark this flag to make the threads end themselves... 
     */
    inform_user("* Client exit\n");
    gDoExit = 1;
}

