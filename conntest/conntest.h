#ifndef CONNTEST_H__
#define CONNTEST_H__
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
#ifdef __cplusplus
extern "C" {
#endif                          /* __cplusplus */
#if 0
}
#endif

/*============================================================================*/
/* INCLUDES                                                                   */
/*============================================================================*/
#include <netinet/in.h>
#include <pthread.h>
/*============================================================================*/
/* MACROS                                                                     */
/*============================================================================*/


/*============================================================================*/
/* TYPES                                                                      */
/*============================================================================*/

typedef struct client_data_t {
    int                sock;
    int                clientNum;
    int                port;
    pthread_t          thread;
    pthread_mutex_t    mutex;
    uint64_t           numSinceStart;
    uint64_t           numSinceMeasure;
    struct sockaddr_in serverAddress;
} client_data_t;

typedef struct app_arg_t {
    int      useUdp;            /* Flag if UDP should be used (1) or TCP (0) */
    int      isServer;          /* Flag if app is server (1) or client (0) */
    uint16_t serverPort;        /* ServerPort */
    char     serverIp[0xFF];    /* Server IP address */
    uint32_t wantedThroughput;  /* Requested throughput */
    int      numConnections;    /* Number of connections */
    int      aggregatorPort;    /* Port of Aggregator */
    uint16_t packetSize;        /* Packet size */
    uint16_t usedPacketSize;    /* Packet size used */
    int      processId;         /* Process ID */
} app_arg_t;


extern uint32_t gWantedTp;



int get_aggregator_port(void);


#ifdef __cplusplus
}
#endif                          /* __cplusplus */
#endif  /* CONNTEST_H__ */
