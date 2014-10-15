#!/usr/bin/env python
#
# -*- mode: Python; fill-column: 75; comment-column: 70; -*-
#
#  This file is part of Torpman's Test Tools 
#         https://github.com/ptorpman/threet
# 
#  Torpman's Test Tools is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
# 
#  Torpman's Test Tools is distributed in the hope that it will 
#  be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
# 
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.or/licenses/>.
#
#
#  Usage: tc-kill-remote.py <program>
#

#==============================================================================
# IMPORTS
#==============================================================================
import os
import sys
import subprocess


def kill_program(prog):
    ''' Will find all processes and kill them '''

    output = subprocess.check_output('ps -ef | grep python | grep %s | grep -v grep' % prog, shell = True)

    for line in output.split('\n'):
        items = line.split(' ')
        items = filter(None, items)

        if len(items) < 2: continue
            
        print '* Killing pid: %s' % items[1]
        os.system('kill -15 %s' % items[1])

if __name__ == '__main__':

    kill_program(sys.argv[1])
