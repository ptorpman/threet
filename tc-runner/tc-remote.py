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
# Usage: tc-remote.py <logfile> <command> <args...>
#
import os
import sys
import signal
import subprocess
import psutil

def setup_signal_handling():
    ''' Install a signal handler '''
    signal.signal(signal.SIGTERM, signal_handler)
    signal.signal(signal.SIGINT, signal_handler)

def signal_handler(sig, frame):
    ''' Respond to signals '''
    parent = psutil.Process(os.getpid())

    for child in parent.get_children(recursive = True):
        child.kill()

    exit(0)


def run_process(argv, logfile):
    ''' Execute the process '''
    output = ''
    errors = ''

    with open(logfile, 'w') as aFile:
        p = subprocess.Popen(argv, stdout = aFile, stderr = aFile)
        output = p.communicate()


if __name__ == '__main__':
    setup_signal_handling()
    run_process(sys.argv[2:], sys.argv[1])
    exit(0) 

