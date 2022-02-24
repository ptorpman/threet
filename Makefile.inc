#/****************************************************************************
# * Copyright (c) 2012 Peter Torpman.                                        *
# *                                                                          *
# * Permission is hereby granted, free of charge, to any person obtaining a  *
# * copy of this software and associated documentation files (the            *
# * "Software"), to deal in the Software without restriction, including      *
# * without limitation the rights to use, copy, modify, merge, publish,      *
# * distribute, distribute with modifications, sublicense, and/or sell       *
# * copies of the Software, and to permit persons to whom the Software is    *
# * furnished to do so, subject to the following conditions:                 *
# *                                                                          *
# * The above copyright notice and this permission notice shall be included  *
# * in all copies or substantial portions of the Software.                   *
# *                                                                          *
# * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS  *
# * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF               *
# * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.   *
# * IN NO EVENT SHALL THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,   *
# * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR    *
# * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR    *
# * THE USE OR OTHER DEALINGS IN THE SOFTWARE.                               *
# *                                                                          *
# * Except as contained in this notice, the name(s) of the above copyright   *
# * holders shall not be used in advertising or otherwise to promote the     *
# * sale, use or other dealings in this Software without prior written       *
# * authorization.                                                           *
# ****************************************************************************/
#
#/****************************************************************************
# *  Author: Peter    Torpman (peter at torpman dot se) 2012                 *
# ****************************************************************************/

CFLAGS = -std=gnu99 -Wall -Wextra -Werror -Wno-unused
LIBS =

ifeq ($(plf),)
  CC = gcc
  LD = ld
  CPPFLAGS = -I. 
endif

ifeq ($(plf), nvidia)
  CC = aarch64-linux-gnu-gcc
  LD = aarch64-linux-gnu-ld
  LDFLAGS = --entry main
  LIBS = -lc
endif

ifeq ($(plf), qnx)
  QNX_VER = $(word 2, $(shell $(QNX_HOST)/usr/bin/qconfig -i | grep "version: "))

  QNX_ARCH=aarch64le

  CC = $(QNX_HOST)/usr/bin/aarch64-unknown-nto-qnx$(QNX_VER)-gcc
  LD = $(QNX_HOST)/usr/bin/aarch64-unknown-nto-qnx$(QNX_VER)-ld

  CFLAGS += -B$(QNX_HOST)/usr/bin
  LDFLAGS +=  -L$(QNX_TARGET)/$(QNX_ARCH)/lib  
  CPPFLAGS += -I$(QNX_TARGET)/qnx7/usr/include -DQNX

  LIBS = -Bdynamic -lsocket 
endif

CXXFLAGS = -g -O2 -Wall -Wextra -Werror

INSTALL_DIR   =  install -d -m
INSTALL_FILES =  install -m
DIR_FLAGS     = 775
FILE_FLAGS    = 664
BIN_FLAGS     = 775

# ****************************************************************************/
# DEFAULT TARGETS 
# ****************************************************************************/
%.o: %.c
	@echo "COMPILING $^" ; $(CC) $(CFLAGS) $(CPPFLAGS) -fPIC -c $^
%.o: %.cc
	@echo "COMPILING $^" ; $(CC) $(CXXFLAGS) $(CPPFLAGS) -fPIC -c $^


