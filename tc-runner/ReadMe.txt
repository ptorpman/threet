================================================================================

#####  ####        #####  #    # #    # #    # ###### #####  
  #   #    #       #    # #    # ##   # ##   # #      #    # 
  #   #      ##### #    # #    # # #  # # #  # #####  #    # 
  #   #            #####  #    # #  # # #  # # #      #####  
  #   #    #       #   #  #    # #   ## #   ## #      #   #  
  #    ####        #    #  ####  #    # #    # ###### #    # 

           (c) 2012-2014 Peter Torpman (peter at torpman dot se)
================================================================================


The tc-runner is a framework for executing test cases. The main idea is that one
process - tc-runner - launches tests on machines, then returns the result code
that e.g Jenkins can examine and present a test result.

tc-runner exists in two variants - one written in Python and one in Tcl.


================================================================================
= TC-RUNNER (Python)
================================================================================

tc-runner.py is configured using a json-formatted file - tc-runner.cfg.
tc-runner.py reads test cases from <directory>/<tc-name>. The testcase file is
called tc.py. The contents of this testcase file is pure Python.

tc-runner.py can launch processes on remote machines using tc-remote.py.

The public API is used like this:

    TcRunner().run_remote(host = 'hostA', iface = 'eth0', cmd = 'ls -l', log = '1.log')
    TcRunner().wait(2)
    TcRunner().assert_log(text = '127.0.0.1', log = '3.log')
    TcRunner().stop_remote_processes()

If failures happen, exceptions will be raised.

================================================================================
= TC-RUNNER (Tcl)
================================================================================

tc-runner launches tc-exe processes on local or remote hosts. Theses tc-exe
processes are the actual drivers of the specific test case. 


tc-runner loads a test case specification file in which it is instructed what
to do, e.g which hosts are involved, what to run on which hosts etc.

tc-runner loads a test case script which performs the actual test work, while 
tc-exe is used to launch the programs locally on a (remote) host.

tc-runner loads a test case from a well-know location e.g /usr/local/testcases.

A server in the tc-runner sense means the process to which input is sent.
A client is a process that generates input for the server process.


Usage:
        tc-runner <config> <testcase>

Example:
        tc-runner tcs/runner.cnf 1.0.0

