#!/bin/sh
#
#  This file is part of Torpman's Test Tools 
#      http://sourceforge.net/projects/torptest)
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
# the next line restarts using tclsh \
exec tclsh "$0" "$@"


#------------------------------------------------------------------------------
# GLOBAL VARIABLES
#------------------------------------------------------------------------------
global TCR

# Turn on debug printouts
set TCR(debug) 1

set TCR(pids) ""
set TCR(path) [exec dirname $::argv0 ]

source $TCR(path)/tc-utils.tcl



proc debug { str } {
    global TCR

    if { $TCR(debug) != 1 } {
        return
    }

    puts "*** $str"
}


#------------------------------------------------------------------------------
# Prints Usage string
#------------------------------------------------------------------------------
proc printUsage { } {
    puts "Usage: tc-runner <config-file> <testcase num>"
}

#------------------------------------------------------------------------------
# Validates a testcase
#------------------------------------------------------------------------------
proc validateDir { } {
    global TCR

    set tc $TCR(tc)

    if  { [info exists TCR(root)] == 0 } {
        puts "ERROR: Root directory not set in config"
        return 1
    }

    # The following files are needed in a test case directory
    # tc.tcl

    if { ! [file exists "$TCR(root)/tcs/$tc"] } {
        puts "ERROR: Test case directory is missing!"
        return 1
    }
    
    if { ! [file exists "$TCR(root)/tcs/$tc/tc.tcl"] } {
        puts "ERROR: tc.tcl file is missing!"
        return 1
    }

    return 0
}

#------------------------------------------------------------------------------
# Executes a process on a host
#------------------------------------------------------------------------------
proc run_exe { host log exe } {
    global TCR

    if  { [info exists TCR(host,$host)] == 0 } {
        puts "ERROR: No IP address for host $host"
        return 1
    }
    
    set ip $TCR(host,$host)
    set tc $TCR(tc)
    set tcDir "$TCR(root)/tcs/$tc"
       
    if { ! [file exists $tcDir/logs] } {
            file mkdir $tcDir/logs
    }

    set res [exec rm -f $tcDir/logs/$log]
       
    puts "* Executing \"$exe\" on $ip ..."

    set prog [lindex $exe 0]

    if { [catch {set pd [exec ssh root@$ip "$TCR(path)/tc-exe.tcl" "$TCR(logs)/$log" "$exe" &] } res ] } {
        puts "ERROR: Failed to start process ($res)."
        return 1
    }    

    lappend TCR(pids) "$pd $ip $prog"

    return 0
}

#
# Stop all started processes
#
proc stop_all_procs { } {
    global TCR

    # Wait a bit before killing
    after 5000

    foreach pi $TCR(pids) {
        set ppid [lindex $pi 0]
        set ip [lindex $pi 1]
        set prog [lindex $pi 2]
        puts "* Killing tc-exe ($prog) on IP $ip"
        catch { exec ssh root@$ip "pkill $prog ; \n ; pkill tclsh" } res
        catch { exec kill $ppid } res
    }

    set TCR(pids) ""

    return 0
}

#
# Grep after a number of occurrences in a file 
#
proc grep_check { fileName num what } {

    set x 0
    
    if { $num != 0 } {
        if { ! [file exists $fileName] } {
            return -code 1 "File does not exist!"
        }

        if { [catch {set x [ exec grep "$what" $fileName | wc -l ]} res] } {
            # Failure!
            return -code 1 "grep failure in $fileName. ($res)"
        }

        if { $x != "$num" } {
            return -code 1 "Incorrect number of messages received! (Got $x. Wanted $num)"
        } else {
            return -code 0 "OK"
        }
    } 

    # Number of hits wanted is zero.
    if { ! [file exists $fileName] } {
        # File is not needed if we want zero...
        return -code 0 "OK"
    }


    if { [catch {set x [ exec grep "$what" $fileName | wc -l ]} res] } {
        # No hits! And, this is OK since we don't want any...
        return -code 0 "OK"
    } else {
        return -code 1 "Too many messages received!"
    }
}


#
# Used for waiting...
#
proc wait { msecs {reason ""}} {
    set numSecLoops [ expr $msecs / 1000 ]
    set left [ expr $msecs % 1000 ]

    debug "Waiting $msecs milliseconds ... $reason"

    for { set i 0 } { $i < $numSecLoops } { incr i } {
        after 1000

        if { $i != 0 && [expr $i % 10] == 0 } {
            puts -nonewline "[expr $i / 10]"
        } else {
            puts -nonewline "."
        }
        flush stdout
    }
    
    after $left
    puts "\n"
    flush stdout
}

#------------------------------------------------------------------------------
#
# INPUT HANDLING AND EXECUTION OF TEST CASE
#
#------------------------------------------------------------------------------
#
# Check input
if { $::argc != 2 } {
    printUsage
    exit 1
}


# Remember testcase
set TCR(tc) [lindex $::argv 1]
set tc $TCR(tc)

#------------------------------------------------------------------------------
# Get configuration
#------------------------------------------------------------------------------
if { [catch {set cnfd [ open [lindex $::argv 0] "r"]} res] } {
    puts "ERROR: Could not open config file ($res)"
    exit 1
}
set tmp  [ read $cnfd ]
set res [ eval $tmp ]
close $cnfd

 
#------------------------------------------------------------------------------
# Validate testcase directory
#------------------------------------------------------------------------------
if { [ validateDir ] } {
    puts "ERROR: Validation of test case directory failed."
    exit 1
}


#------------------------------------------------------------------------------
# Execute the test case file
#------------------------------------------------------------------------------
set tcDir     "$TCR(root)/tcs/$tc"
set TCR(logs) "$TCR(root)/tcs/$tc/logs"

if { [catch {set cnfd [ open "$tcDir/tc.tcl" "r"]} res] } {
    puts "ERROR: Could not open config file ($res)"
    puts "* FAILURE $tc"
    exit 1
}

set tmp  [ read $cnfd ]
if { [catch { set res [ eval $tmp ] } tcRes] } {
    puts "ERROR: Execution failed with ($tcRes res=$res)"
    puts "* FAILURE $tc"
    exit 1
}

close $cnfd

puts "* $tc SUCCESS"

#------------------------------------------------------------------------------
# Stop all processes if any left
#------------------------------------------------------------------------------
stop_all_procs











