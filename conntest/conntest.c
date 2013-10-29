/* Copyright (c) 2012  Peter R. Torpman (peter at torpman dot se) 
 
   This file is part of Torpman's Test Tools  
   http://sourceforge.net/projects/torptest) 

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "conntest.h"
#include "server.h"
#include "client.h"
#include "common.h"
#include "debug.h"

/*============================================================================*/
/* FUNCTION DECLARATIONS                                                      */
/*============================================================================*/
static void print_usage(void);
static void print_version(void);
static int send_msg_to_aggregator(uint8_t* msg, int msgLen, char* server_ip);


/*============================================================================*/
/* VARIABLES                                                                  */
/*============================================================================*/
static uint32_t gIsServer   = 0; /* Flag if in server mode */
static uint32_t gServerPort = 7011; /* Server port */
static char     gServerIp[0xFF]; /* Server IP address */
uint32_t        gWantedTp   = 0; /* Wanted throughput */
static uint32_t gNumConn    = 0; /* Number of connections */
static int      gAggrPort   = -1; /* Aggregator port */
/*============================================================================*/
/* FUNCTION DEFINITIONS                                                       */
/*============================================================================*/

int
main(int argc, char** argv)
{
   /* Default server IP */
   strcpy(gServerIp, "127.0.0.1");

  
   /* Check parameters */
   int i = 1;
   int val = 0;
   int procId = -1;
   
   if (argc < 2) {
      print_usage();
      return 0;
   }
  
   while (i < argc) {

      if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
         print_usage();
         return 0;
      }
      else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version")) {
         print_version();
         return 0;
      }
      else if (!strcmp(argv[i], "-s")) {
         gIsServer = 1;
         i++;
         continue;
      }
      else if (!strcmp(argv[i], "-c")) {
         gIsServer = 0;
         i++;
         continue;
      }
      else if (!strcmp(argv[i], "-id")) {
         if (argv[i + 1] == NULL) {
            print_usage();
            return 1;
         }

         if (sscanf(argv[i+1], "%d", &val) != 1) {
            inform_user("ERROR: Incorrect process ID '%s'\n",
                    argv[i+1]);
            return 1;
         }

         procId = val;

         i += 2;
         continue;
      }
      else if (!strcmp(argv[i], "-a")) {
         if (argv[i + 1] == NULL) {
            print_usage();
            return 1;
         }

         if (sscanf(argv[i+1], "%d", &val) != 1) {
            inform_user("ERROR: Incorrect aggregator port '%s'\n",
                    argv[i+1]);
            return 1;
         }

         gAggrPort = val;

         i += 2;
         continue;
      }

      else if (!strcmp(argv[i], "-n")) {
         if (argv[i + 1] == NULL) {
            print_usage();
            return 1;
         }

         if (sscanf(argv[i+1], "%u", &val) != 1) {
            inform_user("ERROR: Incorrect number of connections '%s'\n",
                    argv[i+1]);
            return 1;
         }

         gNumConn = val;

         i += 2;
         continue;
      }
      else if (!strcmp(argv[i], "-t")) {
         if (argv[i + 1] == NULL) {
            print_usage();
            return 1;
         }

         if (sscanf(argv[i+1], "%u", &val) != 1) {
            inform_user("ERROR: Incorrect throughput '%s'\n",
                    argv[i+1]);
            return 1;
         }

         gWantedTp = val;

         i += 2;
         continue;
      }
      else if (!strcmp(argv[i], "-l")) {
         if (argv[i + 1] == NULL) {
            print_usage();
            return 1;
         }

         strcpy(gServerIp, argv[i + 1]);
         i += 2;
         continue;
      }
      else if (!strcmp(argv[i], "-p")) {
         if (argv[i + 1] == NULL) {
            print_usage();
            return 1;
         }

         if (sscanf(argv[i+1], "%u", &val) != 1) {
            inform_user("ERROR: Incorrect server port '%s'\n",
                    argv[i+1]);
            return 1;
         }

         gServerPort = val;

         i += 2;
         continue;
      }
      else {
         inform_user("Bad parameter! (%s)\n", argv[i]);
         print_usage();
         return 1;
      }
   }


   if (procId == -1) {
         inform_user("ERROR: Process ID not specified!\n");
         print_usage();
         return 1;
   }
   
   if (gIsServer) {
      debug_print( "Starting server with %u connections...\n", gNumConn);
      server_start(gServerPort, gNumConn, procId);
   }
   else {
      signal(SIGTERM, client_exit);
      signal(SIGINT, client_exit);
      debug_print( "Starting client with %u connections...\n", gNumConn);
      client_start(gServerIp, gServerPort, gNumConn, gWantedTp, procId);
   }

   return 0;
}



static void
print_usage(void)
{
   inform_user(
           "Usage: conntest [options]\n"
           "  Options:\n"
           "  -id <num>   Process ID.\n"
           "  -s          Server mode\n"  
           "  -c          Client mode\n"
           "  -a <port>   Aggregator port\n"
           "  -n <num>    Number of connections.\n"
           "  -t <kbps>   Throughput to aim for in kbps\n"
           "  -l <hostip> Host name/IP address\n"
           "  -p <port>   Server port number\n"
           "  -v          Display version information\n"
           "  -h          Display this text\n\n"
           "For bug reporting and suggestions, mail peter@torpman.se\n");
}

static void
print_version(void)
{
   inform_user(
               "conntest (Torpman's Test Tools) 0.1\n"
               "Copyright (C) 2012 Peter R. Torpman (peter@torpman.se)\n"
               "This is free software.  \nYou may redistribute copies of it under "
               "the terms of the GNU General Public License \n"
               "<http://www.gnu.org/licenses/gpl.html>.\n"
               "There is NO WARRANTY, to the extent permitted by law.\n");
}


int
get_aggregator_port(void)
{
	return gAggrPort;
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




