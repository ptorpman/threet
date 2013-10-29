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


SUBDIRS := conntest fstest

# ****************************************************************************/
# MAIN TARGETS
# ****************************************************************************/

all %: 
	@for dir in $(SUBDIRS); do \
	  $(MAKE) -C $$dir $@; \
	done

