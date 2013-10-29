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

# Usage: tc-exe.tcl <log> <prog>                                               
global log
global logFd
global fd

set log "[lindex $::argv 0]"
set exe "[lrange $::argv 1 end]"

# If any curly braces exist, we replace then with quotes
set exe [regsub -all "{" $exe "\"" ]
set exe [regsub -all "}" $exe "\"" ]

proc handleRead {} {
    global fd
    global logFd
    global log

    if { [catch {set x [read $fd]} res ] } {
        flush $logFd
        catch {close $logFd}
        catch {close $fd}
        exit 0
    }


    puts $logFd $x
    flush $logFd
    puts -nonewline ""

    if { [eof $fd] } {
        flush $logFd
        catch {close $logFd}
        catch {close $fd}
        exit 0
    }
}

set logFd [open $log "w"]
set fd [open "| $exe 2>@stdout" "r"]
fconfigure $fd -blocking 0 -buffering none
fileevent $fd readable handleRead

vwait forever


#exec sh -c "$exe 2>&1" > $log
