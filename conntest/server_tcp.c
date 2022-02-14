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
static void* tcp_recv_thread(void* data);
static void* tp_thread(void* data);
static int tcp_create_receiver(int sock, int clientNum);
static int accept_new_connection(int fd);
static int tcp_create_server_socket(uint32_t port, uint32_t numConn);

static int check_and_handle_new_connection(int sock);
static int create_server_tcp_socket(uint32_t port, uint32_t numConn);
static int create_tcp_server(uint32_t port, uint32_t numConn);

/*============================================================================*/
/* VARIABLES */
/*============================================================================*/
static int serverSocket = -1;

/*============================================================================*/
/* LOCAL FUNCTION DEFINITIONS */
/*============================================================================*/

int
tcp_create_server(client_data_t** servers, uint32_t basePort, uint32_t numConn)
{
    /* Create one TCP server sockets used for accepting new connections on */

    serverSocket = tcp_create_server_socket(basePort, numConn);
    
    if (serverSocket < 0) {
        inform_user("ERROR: Failed to open TCP server socket!\n");
        return serverSocket;
    }

    return serverSocket;
}

int
tcp_handle_new_connection(void)
{
    struct pollfd fd[1];
    int numConnected = 1;

    fd[0].fd = serverSocket;
    fd[0].events = POLLIN;

    int res = poll(fd, 1, 10);

    if (res < 0) {
        return 0;
    }

    if (res > 0 && fd[0].revents == POLLIN) {
        int cliSock = accept_new_connection(fd[0].fd);
        if (cliSock < 0) {
            inform_user("ERROR: Failed to accept connection!\n");
            return -1;
        }

        if (tcp_create_receiver(cliSock, numConnected) != 0) {
            inform_user("ERROR: Failed to start client thread!\n");
            return -1;
        }

        inform_user("Client connected!\n");
        numConnected++;
    }

    return 0;
}

static int
tcp_create_server_socket(uint32_t port, uint32_t numConn)
{
    int sock = server_create_and_bind_socket(port, SOCK_STREAM);

    if (sock < 0) {
        inform_user("ERROR: Failed to open socket!\n");
        return -1;
    }

    int res = listen(sock, numConn);

    if (res < 0) {
        inform_user("ERROR: Failed to listen on socket!\n");
        return res;
    }

    inform_user("* Server socket bound to port %u\n", port);
    return sock;
}


static int
accept_new_connection(int fd)
{
    /* Accept incoming connections */
    struct sockaddr_in cliAddr;
    uint32_t cliLen = sizeof(struct sockaddr_in);
    int res;

    res = accept(fd, (struct sockaddr*)&cliAddr, &cliLen);

    if (res < 0) {
        inform_user("ERROR: Failed to accept connection!\n");
    }

    inform_user("ACCEPT: %d!\n", res);
        
    return res;
}


static int
tcp_create_receiver(int sock, int clientNum)
{
    int res;
    client_data_t* data = (client_data_t*)malloc(sizeof(client_data_t));

    data->sock = sock;
    data->clientNum = clientNum;
    pthread_mutex_init(&data->mutex, NULL);

    server_add(data);
    
    return pthread_create(&data->thread, NULL, tcp_recv_thread, (void*)data);
}


static void*
tcp_recv_thread(void* data)
{
    client_data_t* cliData = (client_data_t*)data;

    char buff[0xFFFF];
    size_t buffLen = 0xFFFF;
    int res = 0;

    /* Enter loop to handle data from client */
    while (1) {
        struct sockaddr_in sockAddr;
        socklen_t addrLen = 0;
        res = recvfrom(cliData->sock, buff, buffLen, 0, (struct sockaddr*) &sockAddr, &addrLen);

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
            server_remove(cliData);
            free(cliData);
        }
    }

    pthread_exit(cliData);
}
