#  Copyright (c) 2012  Peter Torpman (peter at torpman dot se)
# 
#  This file is part of Torpman's Test Tools 
#         https://github.com/ptorpman/threet
# 
#  Torpman's Test Tools is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
# 
#  CompFrameTorpman's Test Tools is distributed in the hope that it will 
#  be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
# 
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.or/licenses/>.
# 
include ../Makefile.inc

#-----------------------------------------------------------------------------
# conntest - Connection Throughput Test
#-----------------------------------------------------------------------------
RESULT := conntest
RESULT2 := aggregator

SRC  := conntest.c server.c server_tcp.c server_udp.c client.c common.c debug.c
SRC2 := debug.c aggregator.c 

SRC_CC :=	

OBJ   := $(SRC:.c=.o)
OBJ2   := $(SRC2:.c=.o)
OBJ_CC := $(SRC_CC:.cc=.o)


ifneq ($(plf), qnx)
  LIBS += -lpthread
endif

CFLAGS += -DDEBUG -g

# ****************************************************************************/
# MAIN TARGETS
# ****************************************************************************/
all: $(RESULT2) $(RESULT) 

$(RESULT): $(OBJ) $(OBJ_CC) $(HEADERS)
	$(CC) $(OBJ) $(OBJ_CC) $(LDFLAGS) $(LIBS)  -o $@ 

$(RESULT2): $(OBJ2) $(HEADERS)
	$(CC) $(OBJ2) $(OBJ_CC) $(LDFLAGS) $(LIBS)  -o $@ 

# ****************************************************************************/
# DEFAULT TARGETS 
# ****************************************************************************/
%.o: %.c
	$(CC) -fPIC $(CFLAGS) $(CPPFLAGS) -c $^

%.o: %.cc
	$(CXX) -fPIC $(CXXFLAGS) $(CPPFLAGS) -c $^

clean: 
	-rm *.o *.so $(RESULT) $(RESULT2) *~
