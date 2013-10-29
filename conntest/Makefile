#  Copyright (c) 2012  Peter R. Torpman (peter at torpman dot se)
# 
#  This file is part of Torpman's Test Tools 
#      http://sourceforge.net/projects/torptest)
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


SRC  := conntest.c server.c client.c common.c debug.c
SRC2 := aggregator.c debug.c

SRC_CC :=	

OBJ   := $(SRC:.c=.o)
OBJ2   := $(SRC2:.c=.o)
OBJ_CC := $(SRC_CC:.cc=.o)

LIBS += -lpthread

#CFLAGS += -DDEBUG

# ****************************************************************************/
# MAIN TARGETS
# ****************************************************************************/

all: $(RESULT2) $(RESULT) 

$(RESULT): $(OBJ) $(OBJ_CC) $(HEADERS)
	@echo "[LD] $@" ; $(CC) -rdynamic $(OBJ) $(OBJ_CC) -o $@ $(LIBS) 

$(RESULT2): $(OBJ2) $(HEADERS)
	@echo "[LD] $@" ; $(CC) -rdynamic $(OBJ2) -o $@ $(LIBS) 



# ****************************************************************************/




# ****************************************************************************/
# DEFAULT TARGETS 
# ****************************************************************************/
%.o: %.c
	@echo "[CC] $^" ; $(CC) -fPIC $(CFLAGS) $(CPPFLAGS) -c $^

%.o: %.cc
	@echo "[C++] $^" ; $(CXX) -fPIC $(CXXFLAGS) $(CPPFLAGS) -c $^

clean: 
	-rm *.o *.so $(RESULT) *~
