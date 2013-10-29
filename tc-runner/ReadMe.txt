================================================================================

#####  ####        #####  #    # #    # #    # ###### #####  
  #   #    #       #    # #    # ##   # ##   # #      #    # 
  #   #      ##### #    # #    # # #  # # #  # #####  #    # 
  #   #            #####  #    # #  # # #  # # #      #####  
  #   #    #       #   #  #    # #   ## #   ## #      #   #  
  #    ####        #    #  ####  #    # #    # ###### #    # 

           (c) 2012 Peter R. Torpman (peter at torpman dot se)
================================================================================


The tc-runner is a framework for executing test cases. The main idea is that one
process - tc-runner - launches tests on machines, then returns the result code
that e.g Jenkins can examine and present a test result.

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

