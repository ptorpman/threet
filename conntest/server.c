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
static int create_client_receiver(int sock, int clientNum);
static int create_server_tcp_socket(uint32_t port, uint32_t numConn);
static int create_and_bind_socket(int port, int proto);
static int create_udp_servers(uint32_t basePort, uint32_t numPorts);
static int create_tcp_server(uint32_t port, uint32_t numConn);

/*============================================================================*/
/* VARIABLES */
/*============================================================================*/
static app_arg_t* gAppArgs = NULL;
static client_data_t* gServers[0xFFFF];

static int gSocket = -1;
static pthread_t gTpThread;
static pthread_mutex_t gMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t gMutexRecv = PTHREAD_MUTEX_INITIALIZER;
static uint64_t gRecvBytes = 0;
static uint64_t gRecvBytesSinceStart = 0;

/*============================================================================*/
/* LOCAL FUNCTION DEFINITIONS */
/*============================================================================*/

static int
handle_new_connection(int fd)
{
    /* Accept incoming connections */
    struct sockaddr_in cliAddr;
    uint32_t cliLen = sizeof(struct sockaddr_in);
    int res;

    res = accept(fd, (struct sockaddr*)&cliAddr, &cliLen);

    if (res < 0) {
        inform_user("ERROR: Failed to accept connection!\n");
    }

    return res;
}

static int
create_client_receiver(int sock, int clientNum)
{
    int res;
    client_data_t* cliData = (client_data_t*)malloc(sizeof(client_data_t));

    cliData->sock = sock;
    cliData->clientNum = clientNum;
    pthread_mutex_init(&cliData->mutex, NULL);

    return pthread_create(&cliData->thread, NULL, recv_thread, (void*)cliData);
}

static int
check_and_handle_new_connection(int sock)
{
    struct pollfd fd[1];
    int numConnected = 0;

    fd[0].fd = gSocket;
    fd[0].events = POLLIN;

    int res = poll(fd, 1, 10);

    if (res < 0) {
        return 0;
    }

    if (res > 0 && fd[0].revents == POLLIN) {
        int cliSock = handle_new_connection(fd[0].fd);
        if (cliSock < 0) {
            inform_user("ERROR: Failed to accept connection!\n");
            return -1;
        }

        if (create_client_receiver(cliSock, numConnected) != 0) {
            inform_user("ERROR: Failed to start client thread!\n");
            return -1;
        }

        numConnected++;
    }

    return 0;
}

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
            if (check_and_handle_new_connection(gSocket) < 0) {
                return;
            } else {
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

static int
create_and_bind_socket(int port, int proto)
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

static int
create_server_tcp_socket(uint32_t port, uint32_t numConn)
{
    gSocket = create_and_bind_socket(port, SOCK_STREAM);

    if (gSocket < 0) {
        inform_user("ERROR: Failed to open socket!\n");
        return -1;
    }

    int res = listen(gSocket, numConn);

    if (res < 0) {
        inform_user("ERROR: Failed to listen on socket!\n");
        return res;
    }

    inform_user("* Server socket bound to port %u\n", port);
    return 0;
}

static int
create_tcp_server(uint32_t port, uint32_t numConn)
{
    gServers[0] = (client_data_t*)malloc(sizeof(client_data_t));
    memset(gServers[0], 0, sizeof(client_data_t));

    gServers[0]->sock = create_server_tcp_socket(port, numConn);

    if (gServers[0]->sock < 0) {
        return -1;
    }

    return 0;
}

static int
create_udp_servers(uint32_t basePort, uint32_t numPorts)
{
    for (uint32_t i = 0; i < numPorts; i++) {
        gServers[i] = (client_data_t*)malloc(sizeof(client_data_t));
        memset(gServers[i], 0, sizeof(client_data_t));

        gServers[i]->clientNum = i;

        pthread_mutex_init(&gServers[i]->mutex, NULL);

        gServers[i]->port = basePort + i;
        gServers[i]->sock = create_and_bind_socket(gServers[i]->port, SOCK_DGRAM);

        if (gServers[i]->sock < 0)
        {
            return -1;
        }

        gServers[i]->numSinceStart = 0;
        gServers[i]->numSinceMeasure = 0;

        if (pthread_create(&gServers[i]->thread, NULL, recv_thread, (void*)gServers[i]) < 0) {
            return -1;
        }
    }

    return 0;
}

/*============================================================================*/
/* GLOBAL FUNCTION DEFINITIONS */
/*============================================================================*/

void server_start(app_arg_t* args)
{
    gAppArgs = args;
    int res = -1;

    if (!args->useUdp) {
        if (create_tcp_server(args->serverPort, args->numConnections) < 0) {
            inform_user("ERROR: Failed to open TCP server socket!\n");
            return;
        }
    } else {
        if (create_udp_servers(args->serverPort, args->numConnections) < 0) {
            inform_user("ERROR: Failed to create UDP servers!\n");
        }
    }

    /* Enter eternal loop */
    eternal_loop();
}
