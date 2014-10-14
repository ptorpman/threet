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
#include "common.h"
#include <sys/socket.h>
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



