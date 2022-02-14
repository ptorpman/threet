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

static void* udp_recv_thread(void* data);

/*============================================================================*/
/* VARIABLES */
/*============================================================================*/

/*============================================================================*/
/* LOCAL FUNCTION DEFINITIONS */
/*============================================================================*/


int
udp_create_servers(client_data_t** servers, uint32_t basePort, uint32_t numPorts)
{
    for (uint32_t i = 0; i < numPorts; i++) {
        client_data_t* data = (client_data_t*)malloc(sizeof(client_data_t));
        memset(data, 0, sizeof(client_data_t));

        data->clientNum = i;
        pthread_mutex_init(&data->mutex, NULL);

        data->port = basePort + i;
        data->sock = server_create_and_bind_socket(data->port, SOCK_DGRAM);

        if (data->sock < 0)
        {
            return -1;
        }

        data->numSinceStart = 0;
        data->numSinceMeasure = 0;

        if (pthread_create(&data->thread, NULL, udp_recv_thread, (void*)data) < 0) {
            return -1;
        }

        servers[i] = data;
    }

    return 0;
}

static void*
udp_recv_thread(void* data)
{
    client_data_t* cliData = (client_data_t*)data;

    char buff[0xFFFF];
    size_t buffLen = 0xFFFF;
    int res = 0;
    struct sockaddr_in sockAddr;
    socklen_t addrLen;

    /* Enter loop to handle data from client */
    while (1) {
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
        }
    }

    pthread_exit(cliData);
}


