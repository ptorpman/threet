#ifndef COMMON_H__
#define COMMON_H__

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
#include <stdint.h>
#include "conntest.h"


/** This function is used to register with the aggregator */
int
aggregator_register(int proc_id, char* server_ip);

/** This function is used to report to the aggregator */
int
aggregator_report(int proc_id, uint32_t tput, char* server_ip);

/** Reports statistics */
double
report_statistics(struct timeval* startTime, struct timeval* measurementStartTime,
                  client_data_t** data, uint32_t numData, int id);



#endif  /* COMMON_H__ */
