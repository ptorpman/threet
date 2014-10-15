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
#  Usage: tc-runner.py <tc>
#

#==============================================================================
# IMPORTS
#==============================================================================
import os
import sys
import subprocess
import json
import time

#==============================================================================
# CLASSES
#==============================================================================

class TcRunner(object):
    ''' Main Test Case Runner '''


    # Singleton
    _instance = None
    
    #==============================================================================
    # INTERNAL METHODS
    #==============================================================================

    def __new__(cls, *args, **kwargs):
        if not cls._instance:
            cls._instance = super(TcRunner, cls).__new__(cls, *args, **kwargs)
            cls._instance.config = None
            cls._instance.pids = {}
        return cls._instance

    def __init__(self):
        ''' Constructor '''
        if not self.config:
            self._read_configuration()

    def _read_configuration(self):
        ''' Read and validate configuration '''

        try:
            with open('tc-runner.cfg') as aFile:
                self.config = json.load(aFile, object_hook = self._decode_dict)

        except Exception as exc:
            raise Exception('ERROR: Failed to read configuration file.\nException: %s' % \
                            exc.message)

    def _decode_list(self, data):
        ''' Used to decode unicode characters in json files '''
        rv = []
        for item in data:
            if isinstance(item, unicode):
                item = item.encode('utf-8')
            elif isinstance(item, list):
                item = self._decode_list(item)
            elif isinstance(item, dict):
                item = self._decode_dict(item)
            rv.append(item)
        return rv

    def _decode_dict(self, data):
        ''' Used to decode unicode characters in json files '''
        rv = {}
        for key, value in data.iteritems():
            if isinstance(key, unicode):
                key = key.encode('utf-8')
            if isinstance(value, unicode):
                value = value.encode('utf-8')
            elif isinstance(value, list):
                value = self._decode_list(value)
            elif isinstance(value, dict):
                value = self._decode_dict(value)
            rv[key] = value
        return rv

    def _validate_testcase(self):
        ''' Make sure the testcase can be found '''
        if not os.path.exists(self.tc_file):
            raise Exception('ERROR: Testcase file does not exist: %s' % self.tc_file)

    def _execute(self, tc_name):
        ''' Execute the test case '''

        self.tc_name      = os.path.basename(tc_name)
        self.tc_directory = os.path.dirname(tc_name)
        self.tc_file      = '%s/%s/tc.py' % (self.tc_directory, self.tc_name)
        self.tc_log_dir   = '%s/%s/logs' % (self.tc_directory, self.tc_name)
        
        if not os.path.exists(self.tc_log_dir):
            os.makedirs(self.tc_log_dir)

        self._validate_testcase()

        global_env = globals()
        local_env = {}
        
        try:
            execfile(self.tc_file, global_env, local_env) 
        except Exception as exc:
            raise Exception('ERROR: Testcase %s failed.\n   Exception: %s' % (self.tc_file, exc))

         

    #==============================================================================
    # EXTERNAL API
    #==============================================================================


    def run_remote(self, host, iface, cmd, log, user = None):
        ''' Run a program on a host '''

        try:
            use_ip = self.config['hosts_config'][host]['ip'][iface]
        except Exception as exc:
            try:
                use_ip = self.config['hosts_config'][host]['name']
            except Exception as exc:
                raise Exception('ERROR: Host configuration for %s' % host)            

        tc_remote = '%s/tc-remote.py %s/%s ' % (os.getcwd(), self.tc_log_dir, log)
        use_user  = user if user else os.getlogin()

        cmd = tc_remote + cmd
        
        use_cmd = ['/usr/bin/ssh', '-2', '-X', '%s@%s' % (use_user, use_ip), cmd]

        print "* Running command: %s" % ' '.join(use_cmd)
        pid = os.fork()

        if pid:
            self.pids[pid] = [use_user, use_ip, cmd]
            return

        os.execv(use_cmd[0], use_cmd)
        os._exit(0)


    def stop_remote_processes(self):
        ''' Kill processes launched on the remote machine '''

        if not self.pids:
            return

        for pid in self.pids:
            use_user = self.pids[pid][0]
            use_ip   = self.pids[pid][1]
            used_cmd = self.pids[pid][2].split(' ')[0]

            tc_kill = '%s/tc-kill-remote.py tc-remote.py' % (os.getcwd())

            try:
                print '* Stopping remote process at %s' % use_ip
                output = subprocess.check_output(['/usr/bin/ssh', '-2',
                                                  '%s@%s' % (use_user, use_ip),
                                                  tc_kill], stderr = subprocess.STDOUT)
            except Exception as exc:
                # Do not care...
                pass

            # Wait for the forked process
            (p, status) = os.wait()
        
        self.pids = {}

            
    def wait(self, seconds):
        ''' Suspend execution '''
        print '* Sleeping %s seconds ...' % seconds
        time.sleep(seconds)
        print '* Resuming...'

    def assert_log(self, text, log):
        ''' Examines logfile and raises an exception if specified text is missing '''
        logfile = '%s/%s' % (self.tc_log_dir, log)

        print '* Asserting \"%s\" in %s' % (text, logfile)

        with open(logfile, 'r') as aFile:
            lines = aFile.read()
            if not text in lines:
                raise Exception('ERROR: Text not found in logfile')

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print 'Usage: tc-runner.py <testcase>'
        exit(1)

    try:
        tc_runner = TcRunner()
        tc_runner._execute(sys.argv[1])
        print '* %s OK' % tc_runner.tc_name
    except Exception as exc:
        print '* %s FAILED' % tc_runner.tc_name
        print exc
        exit(1)
    finally:
        tc_runner.stop_remote_processes()
