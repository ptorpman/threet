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
#include "common.h"
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "conntest.h"
#include "debug.h"

static int send_msg_to_aggregator(uint8_t* msg, int msgLen, char* server_ip);


int
aggregator_register(int proc_id, char* server_ip)
{
    /* Send a UDP message to the aggregator to inform that the client
       is ready for action.*/
    uint8_t msg[1];
    msg[0] = proc_id;

    return send_msg_to_aggregator(msg, 1, server_ip);
}


/** This function is used to report to the aggregator */
int
aggregator_report(int proc_id, uint32_t tput, char* server_ip)
{
    uint8_t msg[5];

    msg[0] = proc_id;
    msg[1] = (tput >> 24) & 0xFF;
    msg[2] = (tput >> 16) & 0xFF;
    msg[3] = (tput >> 8) & 0xFF;
    msg[4] = tput & 0xFF;

    return send_msg_to_aggregator(msg, 5, server_ip);
}


static int
send_msg_to_aggregator(uint8_t* msg, int msgLen, char* server_ip)
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
    he = gethostbyname(server_ip);

    if (!he) {
        /* Nope. Check dotted IP... */
        if (inet_aton(server_ip, &inAddr) == 0) {
            debug_print( "ERROR: Unknown server address!\n");
            return -1;
        }
    }
    else {
        inAddr = *((struct in_addr*) he->h_addr);
    }

    sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock < 0) {
        debug_print( "ERROR: Failed to open socket!\n");
        return -1;
    }

    /* Set reusable socket */
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*) &reuse, sizeof(reuse));
   
    sinAddr.sin_family = AF_INET;
    sinAddr.sin_addr = inAddr;
    sinAddr.sin_port = htons(get_aggregator_port());

    res = sendto(sock, msg, msgLen, 0,
                 (struct sockaddr*) &sinAddr, sizeof (struct sockaddr_in));

    if (res < 0) {
        inform_user("ERROR: Failed to send to aggregator!\n");
    }

    return res;
}

double
report_statistics(struct timeval* startTime, struct timeval* measurementStartTime, 
                  client_data_t** data, uint32_t numData, int id)
{
    uint64_t numBytes = 0;
    uint64_t numBytesSinceStart = 0;
    uint32_t elapsed = 0;
    double bitsPerSecNow = 0;
    double bitsPerSecAverage = 0;
    
    for (uint32_t i = 0; i < numData; i++) {
        if (data[i] == NULL || data[i]->sock == 0) {
            /* Not operational yet */
            continue;
        }

        pthread_mutex_lock(&data[i]->mutex);
        numBytes += data[i]->numSinceMeasure;
        data[i]->numSinceMeasure = 0;
        numBytesSinceStart += data[i]->numSinceStart;
        pthread_mutex_unlock(&data[i]->mutex);
    }

    struct timeval timeNow;
    gettimeofday(&timeNow, NULL);
    uint32_t numSecs = (timeNow.tv_sec - startTime->tv_sec) + 
        ((timeNow.tv_usec - startTime->tv_usec) / 1000000);

    /* In seconds since start of traffic */
    elapsed = (timeNow.tv_sec - measurementStartTime->tv_sec) + 
        ((timeNow.tv_usec - measurementStartTime->tv_usec) / 1000000);

    bitsPerSecNow = (numBytes * 8) / elapsed;
    bitsPerSecAverage = (numBytesSinceStart * 8 / 1024) / numSecs;
    
    if (get_aggregator_port () != 0) {
        aggregator_report(id, bitsPerSecNow / 1024, "127.0.0.1");
    }
    else {
        inform_user("Throughput @%3u s: %10.2f kbps %10.2f Mbps (Avg: %10.2f Mbps)\n",
                    numSecs, (bitsPerSecNow / 1024),
                    (bitsPerSecNow / (1024 * 1024)), bitsPerSecAverage / 1024);
    }


    return bitsPerSecNow;
}

