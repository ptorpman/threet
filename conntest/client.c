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
#include "common.h"
#include "conntest.h"
#include "debug.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

/*============================================================================*/
/* FUNCTION DECLARATIONS                                                      */
/*============================================================================*/
static void* send_thread(void* data);
static void* tp_thread(void* data);

/* Returns socket that was connected */
static int connect_to_server(char* serverIp, uint32_t serverPort);
static void adjust_traffic(double tPutNow);
static void allocate_clients(uint32_t num);
static int connect_clients(uint32_t numConn);
static int create_client_threads(uint32_t num);
static int create_client_udp_sockets(uint32_t num);

/*============================================================================*/
/* VARIABLES                                                                  */
/*============================================================================*/
static app_arg_t* gAppArgs = NULL;

static client_data_t* gClients[0xFFFF];
static pthread_mutex_t gMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t gMutexRate = PTHREAD_MUTEX_INITIALIZER;
static int gDoExit = 0;
static pthread_t gTpThread;
static uint8_t gSendBuffer[0xFFFFFF];
static int64_t gBytesToSend = 100;

/*============================================================================*/
/* LOCAL FUNCTION DEFINITIONS                                                 */
/*============================================================================*/

static void
allocate_clients(uint32_t num)
{
    for (int i = 0; i < (int)num; i++) {
        gClients[i] = (client_data_t*)malloc(sizeof(client_data_t));
        pthread_mutex_init(&gClients[i]->mutex, NULL);
    }
}

static int
connect_clients(uint32_t num)
{
    for (int i = 0; i < (int)num; i++) {
        client_data_t* cliData = gClients[i];
        memset(cliData, 0, sizeof(client_data_t));
        cliData->clientNum = i;
        cliData->sock = connect_to_server(gAppArgs->serverIp, gAppArgs->serverPort);
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

    pthread_mutex_lock(&gMutex);

    /* Start througput display thread */
    res = pthread_create(&gTpThread, NULL, tp_thread, NULL);
    if (res < 0) {
        debug_print("ERROR: Failed to start throughput thread!\n");
        return 1;
    }

    for (int i = 0; i < (int)num; i++) {
        pthread_t t;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

        res = pthread_create(&gClients[i]->thread, &attr, send_thread, (void*)gClients[i]);

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
    for (int i = 0; i < (int)num; i++) {
        pthread_join(gClients[i]->thread, NULL);
    }

    return 1;
}

static void*
send_thread(void* data)
{
    client_data_t* cliData = (client_data_t*)data;
    int res = 0;

    pthread_mutex_lock(&gMutex);
    if (gDoExit) {
        pthread_exit(cliData);
    }
    pthread_mutex_unlock(&gMutex);

    uint32_t bytesToSend = gAppArgs->usedPacketSize > 0 ? gAppArgs->packetSize : gBytesToSend;
    size_t addrSize = sizeof(struct sockaddr_in);

    while (1) {
        usleep(10);

        if (gDoExit) {
            debug_print("Client exit\n");
            close(cliData->sock);
            pthread_exit(cliData);
        }

        if (!gAppArgs->useUdp) {
            res = send(cliData->sock, gSendBuffer, bytesToSend, MSG_NOSIGNAL);
        }
        else {
            res = sendto(cliData->sock, gSendBuffer, bytesToSend, 0, (struct sockaddr*)&cliData->serverAddress, addrSize);
        }

        if (res > 0) {
            cliData->numSinceStart += res;
            cliData->numSinceMeasure += res;
        }
    }
}

static void*
tp_thread(void* data)
{
    /*
     * This thread is used for displaying the current throughput. 
     */

    struct timeval startTime;
    struct timeval measurementStartTime;

    gettimeofday(&startTime, NULL);

    while (1) {

        /* Sleep 1 second */
        gettimeofday(&measurementStartTime, NULL);
        usleep(1000000);
        if (gDoExit) {
            break;
        }

        double bitsPerSecNow = report_statistics(&startTime, &measurementStartTime,
                                                 gClients, gAppArgs->numConnections,
                                                 gAppArgs->processId);

        if (gAppArgs->packetSize == 0) {
            adjust_traffic(bitsPerSecNow);
        }
    }

    pthread_exit(NULL);
}

static void
adjust_traffic(double tPutNow)
{
    double diff;

    if (gAppArgs->wantedThroughput > 0 && (tPutNow / 1024) > gAppArgs->wantedThroughput) {
        diff = (tPutNow / 1024) - gAppArgs->wantedThroughput; /* in kbps */

        /*
         * Allow 1 percent diff from wanted throughput 
         */
        if (diff > (0.01 * gAppArgs->wantedThroughput)) {

            /*
             * Throughput is too high. Try to decrease it 
             */
            pthread_mutex_lock(&gMutexRate);
            gBytesToSend = gBytesToSend - ((0.25 * diff) / gAppArgs->numConnections);
            if (gBytesToSend < 0) {
                gBytesToSend = 1;
            }
            pthread_mutex_unlock(&gMutexRate);
        }
    }

    else if (gAppArgs->wantedThroughput > 0 && (tPutNow / 1024) < gAppArgs->wantedThroughput) {
        diff = gAppArgs->wantedThroughput - (tPutNow / 1024); /* in kbps */

        /*
         * Allow 1 percent diff from wanted throughput 
         */
        if (diff > (0.01 * gAppArgs->wantedThroughput)) {

            /*
             * Throughput is too low. Try to increase it 
             */
            pthread_mutex_lock(&gMutexRate);
            gBytesToSend = gBytesToSend + ((0.25 * diff) / gAppArgs->numConnections);
            pthread_mutex_unlock(&gMutexRate);
        }
    }
}

static int
connect_to_server(char* serverIp, uint32_t serverPort)
{
    /*
     * Get server address 
     */
    struct hostent* he;
    struct in_addr inAddr;
    int sock = -1;
    int reuse = 1;
    int res = 0;
    struct sockaddr_in sinAddr;
    memset(&sinAddr, 0, sizeof(struct sockaddr_in));

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

    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));

    sinAddr.sin_family = AF_INET;
    sinAddr.sin_addr = inAddr;
    sinAddr.sin_port = htons(serverPort);

    res = connect(sock, (struct sockaddr*)&sinAddr, sizeof(struct sockaddr));
    if (res < 0) {
        debug_print("ERROR: Failed to connect socket! errno=%d\n", errno);
        perror("Could not connect socket");
        return -1;
    }
    return sock;
}

static int create_client_udp_sockets(uint32_t num)
{
    for (uint32_t i = 0; i < num; i++) {
        gClients[i]->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        if (gClients[i]->sock < 0) {
            return -1;
        }

        struct sockaddr_in* sinAddr = &gClients[i]->serverAddress;
        sinAddr->sin_family = AF_INET;
        sinAddr->sin_port = htons(gAppArgs->serverPort + i);
        inet_pton(AF_INET, gAppArgs->serverIp, &sinAddr->sin_addr.s_addr);
    }

    return 0;
}

/*============================================================================*/
/* GLOBAL FUNCTION DEFINITIONS                                                */
/*============================================================================*/

void client_start(app_arg_t* args)
{
    gAppArgs = args;
    int res = 0;

    memset(gSendBuffer, 0xFF, 0xFFFFFF);

    if (gAppArgs->packetSize > 0) {
        inform_user("* Will send packets of size: %d\n", gAppArgs->packetSize);
        gAppArgs->wantedThroughput = 0;
        gAppArgs->usedPacketSize = gAppArgs->packetSize - ETH_HEADER_SIZE;
    } else {
        inform_user("* Will aim for throughput: %d\n", gAppArgs->wantedThroughput);
    }

    allocate_clients(gAppArgs->numConnections);

    if (!gAppArgs->useUdp) {
        if (connect_clients(gAppArgs->numConnections) != 0) {
            inform_user("* Failed to connect clients\n");
            return;
        }
    }
    else {
        if (create_client_udp_sockets(gAppArgs->numConnections) != 0) {
            inform_user("* Failed to open client sockets\n");
            return;
        }
    }

    if (create_client_threads(gAppArgs->numConnections) != 0) {
        inform_user("* Failed to create client threads\n");
        return;
    }

    if (get_aggregator_port() != 0) {
        aggregator_register(gAppArgs->processId, "127.0.0.1");
    }

    debug_print("Clients running (conns=%u)...(tp=%u kbps pktsize=%u)\n",
                gAppArgs->numConnections,
                gAppArgs->wantedThroughput,
                gAppArgs->packetSize);

    while (1) {
        usleep(1000);
        if (gDoExit) {
            break;
        }
    }
}

void client_exit(int sig)
{
    /*
     * Mark this flag to make the threads end themselves... 
     */
    inform_user("* Client exit\n");
    gDoExit = 1;
}
