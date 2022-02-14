#ifndef SERVER_H__
#define SERVER_H__
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
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#include "conntest.h"
/*============================================================================*/
/* MACROS                                                                     */
/*============================================================================*/


/*============================================================================*/
/* TYPES                                                                      */
/*============================================================================*/

void server_start(app_arg_t* args);
int server_create_and_bind_socket(int port, int proto);

void server_add(client_data_t* data);
void server_remove(client_data_t* data);

int tcp_create_server(client_data_t** servers, uint32_t basePort, uint32_t numConn);
int tcp_handle_new_connection(void);

int udp_create_servers(client_data_t** servers, uint32_t basePort, uint32_t numPorts);


#ifdef __cplusplus
}
#endif                          /* __cplusplus */
#endif  /* SERVER_H__ */
