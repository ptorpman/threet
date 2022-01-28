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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include "debug.h"

static int gNumClients = -1;
static int gNumRegistered = 0;
static uint32_t gClientTp[0xFF];
static int gDoReport = 0;
static int gClientReported[0xFF];
static int gPort = -1;
static double gAvgTput = 0;
static int gNumMeasurements = 60;

static void print_usage(void);
static int start_aggregating(void);

static void
print_usage(void)
{
    inform_user(
        "Usage: aggregator [options]\n"
        "  Options:\n"
        "  -n <num>    Number of clients/servers to aggregate\n"
        "  -p <port>   Port number to listen to\n"
        "  -m <num>    Number of measurements to receive (Default: 60)\n"
        "  -h          Display this text\n\n"
        "For bug reporting and suggestions, mail peter@torpman.se\n");
}



int
main(int argc, char** argv)
{
    memset(gClientTp, 0, sizeof(gClientTp));
    memset(gClientReported, 0, sizeof(gClientReported));

   
    /* Check parameters */
    int i = 1;
    int val = 0;
   
    if (argc < 2) {
        print_usage();
        return 0;
    }

    while (i < argc) {

        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            print_usage();
            return 0;
        }
        else if (!strcmp(argv[i], "-n")) {
            if (argv[i + 1] == NULL) {
                print_usage();
                return 1;
            }

            if (sscanf(argv[i+1], "%d", &val) != 1) {
                inform_user("ERROR: Incorrect number of clients/servers '%s'\n",
                            argv[i+1]);
                return 1;
            }

            gNumClients = val;

            i += 2;
            continue;
        }
        else if (!strcmp(argv[i], "-p")) {
            if (argv[i + 1] == NULL) {
                print_usage();
                return 1;
            }

            if (sscanf(argv[i+1], "%d", &val) != 1) {
                inform_user("ERROR: Incorrect port number '%s'\n",
                            argv[i+1]);
                return 1;
            }

            gPort = val;

            i += 2;
            continue;
        }
        else if (!strcmp(argv[i], "-m")) {
            if (argv[i + 1] == NULL) {
                print_usage();
                return 1;
            }

            if (sscanf(argv[i+1], "%d", &val) != 1) {
                inform_user("ERROR: Incorrect measurement limit '%s'\n",
                            argv[i+1]);
                return 1;
            }

            gNumMeasurements = val;

            i += 2;
            continue;
        }
        else {
            inform_user("Bad parameter! (%s)\n", argv[i]);
            print_usage();
            return 1;
        }
    }
   
    if (gPort == -1) {
        inform_user("Bad port number! (%d)\n", gPort);
        print_usage();
        return 1;
    }

    return start_aggregating();
}

static int
start_aggregating(void)
{
    int res = 0;
   
    /* Open server socket */
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock < 0) {
        inform_user("ERROR: Failed to open socket!\n");
        return 1;
    }

    struct sockaddr_in sinAddr;
    memset(&sinAddr, 0, sizeof(struct sockaddr_in));

    sinAddr.sin_family      = AF_INET;
    sinAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    sinAddr.sin_port        = htons(gPort);
   
    res = bind (sock, (struct sockaddr*) &sinAddr, sizeof(struct sockaddr_in));

    if (res < 0) {
        inform_user("ERROR: Failed to bind socket!\n");
        return 1;
    }

    /* Start polling */
    struct pollfd fds[1];
    fds[0].fd = sock;
    fds[0].events = POLLIN;

    struct sockaddr_in cliAddr;
    socklen_t len = sizeof(struct sockaddr_in);
    int n = 0;
    uint8_t msg[10];
    int reportNum = 0;

    inform_user("* Aggregator started on port %d. Clients: %d\n",
                gPort, gNumClients);
   
    while (1) {
        res = poll(fds, 1, -1);

        if (res > 0 && (fds[0].revents == POLLIN)) {
            /* Got stuff on socket */
            n = recvfrom(fds[0].fd, msg, 10, 0, (struct sockaddr*) &cliAddr, &len);

            if (n == 1) {
                /* Register message */
                gNumRegistered++;

                if (gNumRegistered == gNumClients) {
                    gDoReport = 1;
                }
                continue;
            }


            if (n == 5) {
                /* A throughput message*/
                uint32_t tp = (msg[1] << 24) + (msg[2] << 16) + (msg[3] << 8) + msg[4];
                uint32_t totTp = 0;
                int doPrint = 1;
            
                gClientReported[msg[0]] = 1;
                gClientTp[msg[0]] = tp;

                /* Have all reported? */
                for (int i = 0; i < gNumClients; i++) {
                    if (!gClientReported[i]) {
                        /* Nope. Wait for all clients to report... */
                        doPrint = 0;
                        break;
                    }

                    totTp += gClientTp[i];
                }

                if (doPrint) {
                    /* All have reported now print! */
                    gAvgTput += totTp;

                    if (gAvgTput == 0) {
                        /* We skip printing the intial values if they are all zero.
                           because it messes up the average (on server side). */
                        memset(gClientTp, 0, sizeof(gClientTp));
                        memset(gClientReported, 0, sizeof(gClientReported));
                        continue;
                    }

                    reportNum++;
                    inform_user("* #%-4d Aggregated throughput: %u kbps (%0.2f Mbps)"
                                " Avg: %0.2f Mbps\n",
                                reportNum, totTp,
                                (double) totTp / 1024,
                                (gAvgTput / (reportNum) / 1024));
                    memset(gClientTp, 0, sizeof(gClientTp));
                    memset(gClientReported, 0, sizeof(gClientReported));

                    if (reportNum == gNumMeasurements) {
                        /* We have reached the limit. */
                        int resX = system("pkill conntest"); /* Kill the clients */
                        inform_user("* Measurement limit (%d) reached. Exiting...\n", gNumMeasurements);
                        return 0;
                    }

                }
            }
        }
    }

    return 0;
}

