#ifndef CLIENT_H__
#define CLIENT_H__
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

/*============================================================================*/
/* MACROS                                                                     */
/*============================================================================*/


/*============================================================================*/
/* TYPES                                                                      */
/*============================================================================*/

void client_start(char* serverIp, uint32_t serverPort,
                  uint32_t numConn, uint32_t wantedTp, int client_id);
void client_exit(int sig);

#ifdef __cplusplus
}
#endif                          /* __cplusplus */
#endif  /* CLIENT_H__ */
