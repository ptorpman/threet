#ifndef FSTEST_H__
#define FSTEST_H__
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

/*============================================================================*/
/* MACROS                                                                     */
/*============================================================================*/


/*============================================================================*/
/* TYPES                                                                      */
/*============================================================================*/


/* Type of test*/
typedef enum {
   TEST_TYPE_READ   = 0,
   TEST_TYPE_WRITE  = 1
} fs_testtype_t;




#ifdef __cplusplus
}
#endif                          /* __cplusplus */
#endif  /* FSTEST_H__ */
