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
/* INCLUDES */
/*============================================================================*/
#include "server.h"
#include <netinet/in.h>
#include <poll.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define _REENTRANT
#include <pthread.h>

#include "common.h"
#include "conntest.h"
#include "debug.h"

/*============================================================================*/
/* FUNCTION DECLARATIONS */
/*============================================================================*/
static void eternal_loop(void);
static void* recv_thread(void* data);
static void* tp_thread(void* data);

static int check_and_handle_new_connection(int sock);
static int handle_new_connection(int fd);
static int create_server_tcp_socket(uint32_t port, uint32_t numConn);
static int create_and_bind_socket(int port, int proto);
static int create_tcp_server(uint32_t port, uint32_t numConn);

/*============================================================================*/
/* VARIABLES */
/*============================================================================*/
static app_arg_t* gAppArgs = NULL;
static client_data_t* gServers[0xFFFF];

static pthread_t gTpThread;
static pthread_mutex_t gMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t gMutexRecv = PTHREAD_MUTEX_INITIALIZER;
static uint64_t gRecvBytes = 0;
static uint64_t gRecvBytesSinceStart = 0;

static int gAdded = 0;

/*============================================================================*/
/* LOCAL FUNCTION DEFINITIONS */
/*============================================================================*/

static void
eternal_loop(void)
{
    /* Start throughput display thread */
    int res = pthread_create(&gTpThread, NULL, tp_thread, NULL);

    if (res < 0) {
        inform_user("ERROR: Failed to start througput thread!\n");
        return;
    }

    if (get_aggregator_port() != 0) {
        aggregator_register(gAppArgs->processId, "127.0.0.1");
    }

    while (1) {
        if (!gAppArgs->useUdp) {
            if (tcp_handle_new_connection() < 0) {
                return;
            }
            else {
            }
        } else {
            usleep(10000);
        }
    }
}

static void*
recv_thread(void* data)
{
    client_data_t* cliData = (client_data_t*)data;

    char buff[0xFFFF];
    size_t buffLen = 0xFFFF;
    int res = 0;

    /* Enter loop to handle data from client */
    while (1) {
        if (!gAppArgs->useUdp) {
            res = recv(cliData->sock, buff, buffLen, 0);
        }
        else {
            struct sockaddr_in sockAddr;
            socklen_t addrLen;
            res = recvfrom(cliData->sock, buff, buffLen, 0, (struct sockaddr*) &sockAddr, &addrLen);
        }

        if (res > 0) {
            /* Update number of received bytes */
            pthread_mutex_lock(&cliData->mutex);
            cliData->numSinceMeasure += res;
            cliData->numSinceStart += res;
            pthread_mutex_unlock(&cliData->mutex);
        }

        if (res == 0) {
            /* Remote side has closed down... */
            pthread_exit(cliData);
        }

    }

    pthread_exit(cliData);
}

static void*
tp_thread(void* data)
{
    /* This thread is used for displaying the current throughput. */
    struct timeval startTime;
    struct timeval measurementStartTime;

    gettimeofday(&startTime, NULL);

    while (1) {
        /* Sleep 1 second */
        gettimeofday(&measurementStartTime, NULL);
        usleep(1000000);

        report_statistics(&startTime, &measurementStartTime, gServers,
                          gAppArgs->numConnections, gAppArgs->processId);
    }

    pthread_exit(NULL);
}

/*============================================================================*/
/* GLOBAL FUNCTION DEFINITIONS */
/*============================================================================*/

void server_start(app_arg_t* args)
{
    for(int i = 0; i < 0xFFFF; i++) {
        gServers[i] = NULL;
    }

    gAppArgs = args;
    int res = -1;

    if (!args->useUdp) {
        if (tcp_create_server(gServers, args->serverPort, args->numConnections) < 0) {
            inform_user("ERROR: Failed to open TCP server socket!\n");
            return;
        }
    } else {
        if (udp_create_servers(gServers, args->serverPort, args->numConnections) < 0) {
            inform_user("ERROR: Failed to create UDP servers!\n");
        }
    }

    /* Enter eternal loop */
    eternal_loop();
}


int
server_create_and_bind_socket(int port, int proto)
{
    int sock = 0;

    if (proto == SOCK_DGRAM) {
        sock = socket(AF_INET, proto, IPPROTO_UDP);
    } else {
        sock = socket(AF_INET, proto, 0);
    }

    if (sock < 0) {
        inform_user("ERROR: Failed to open socket!\n");
        return -1;
    }

    /* Set reusable socket */
    int reuse = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));

    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(struct sockaddr_in));

    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(port);

    int res = bind(sock, (struct sockaddr*)&servAddr, sizeof(servAddr));

    if (res < 0) {
        inform_user("ERROR: Failed to bind socket!\n");
        return res;
    }

    return sock;
}

void
server_add(client_data_t* data)
{
    gServers[++gAdded] = data;
}

void
server_remove(client_data_t* data)
{
    for(int i = 0; i < 0xFFFF; i++) {
        if (gServers[i] == data) {
            gServers[i] = NULL;
            return;
        }
    }
}

