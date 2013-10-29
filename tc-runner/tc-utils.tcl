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
#
#  This file contains utility functions.
#

#
# Returns an IPv4 address for an interface
#
proc tcr_ip_addr_get { ip iface } {
    if { [catch {set str [exec ssh root@$ip "/sbin/ifconfig $iface"] } res ] } {
        return -code 1 "Could not get interface info for $iface on $ip."
    }    
    
    # Extract the address
    set idx1 [string first "inet addr:" $str ]
    set idx2 [string first "Bcast:" $str ]

    set addr [string trim [string range $str [expr $idx1 + 10] [expr $idx2 - 1]]]

    debug "Device $iface on $ip has address $addr"

    return $addr
}

#
# Returns the first global IPv6 address for an interface
#
proc tcr_ipv6_addr_get { ip iface } {
    if { [catch {set str [exec ssh root@$ip "/sbin/ifconfig $iface"] } res ] } {
        return -code 1 "Could not get interface info for $iface on $ip. ($res)"
    }    
    
    set idx1 [ string first "inet6 addr:" $str]
    set idx2 [ string first "Scope:Global" $str ] 

    if { $idx1 == -1 || $idx2 == -1 } {
        return ""
    }

    set str [ string range $str [expr $idx1 + 11] [expr $idx2 - 1] ]

    # Check if a non-global is in there...
    set idx1 [ string first "inet6 addr:" $str]

    if { $idx1 != -1 } {
        set str [ string range $str [expr $idx1 + 11] [expr $idx2 - 1] ]
    }

    debug "Device $iface on $ip has address $str"
    return [string trim $str]
}


# 
# Checks that an IPv4 address of an interface is within the correct range
#
proc tcr_ip_check_range { host iface rangeLow rangeHigh } {
    set str ""
    set res ""
    
    debug "Checking IP range: $host $iface $rangeLow $rangeHigh"

    if { [catch {set str [exec ssh root@$host "/sbin/ifconfig $iface"] } res ] } {
        return -code 1 "Could not get interface info for $iface on $host."
    }    
    
    # Extract the address
    set idx1 [string first "inet addr:" $str ]
    set idx2 [string first "Bcast:" $str ]

    set addr [string trim [string range $str [expr $idx1 + 10] [expr $idx2 - 1]]]

    set idx1 [string last "." $addr]
    set num  [string trim [string range $addr [expr $idx1 + 1] end]]

    # Within the correct range?
    if { ! ( $addr >= $rangeLow && $addr <= $rangeHigh)  } {
        return -code 1 "Bad number ($addr) ($rangeLow <= $addr && $addr <= $rangeHigh)"
    }
    
    debug "OK $addr"
    return -code 0 "OK"
}

# 
# Checks that an IPv6 address of an interface is within the correct range
# (Last digits implemented, and initial part of address is verified)
#
proc tcr_ipv6_check_range { host iface rangeLow rangeHigh } {
    set str ""
    set res ""
    
    debug "Checking IP range: $host $iface $rangeLow $rangeHigh"

    # "2001:db8:0:e101::10" "2001:db8:0:e101::100" 

    set addr [tcr_ipv6_addr_get $host $iface]

    if { $addr == "" } {
        return -code 1 "Could not get IPv6 address on $host"
    }

    debug "$host $iface has address $addr"
    
    # Check that the address start the same

    set startRangeLow  [string range $rangeLow 0 [ string last ":" $rangeLow]]
    set startAddr      [string range $addr     0 [ string last ":" $addr]]

    if { $startRangeLow != $startAddr } {
        return -code 1 "Ranges are not correct ($startAddr) != ($startRangeLow)"
    }

    # Get past the / and get the last digit portion of the address
    set addr [ string range $addr 0 [expr [string first "/" $addr] - 1]]
    set idx  [ string last ":" $addr ]
    set addr [ string range $addr [expr $idx + 1] end ]

    set rangeLow  [ string range $rangeLow  [expr [string last ":" $rangeLow]  + 1] end ]
    set rangeHigh [ string range $rangeHigh [expr [string last ":" $rangeHigh] + 1] end ]



    # Within the correct range?
    if { ! ( $addr >= $rangeLow && $addr <= $rangeHigh)  } {
        return -code 1 "Bad number ($addr) ($rangeLow <= $addr && $addr <= $rangeHigh)"
    }
    
    return -code 0 "OK"
}


# 
# Checks that the correct gateway is used towards a network
#
proc tcr_gw_check { ip iface gw remoteAddr } {

    debug "Checking gateway: $ip $iface $gw $remoteAddr"

    if { [catch {set str [exec ssh root@$ip "traceroute -i $iface $remoteAddr"] } res ] } {
        return -code 1 "Could not get route info to $remoteAddr ($res)"
    }    

    # Remove header
    set str [string trim [string range $str [expr [string first "packets" $str] + 7] end]]
    
    set idx1 [string first "(" $str ]
    set idx2 [string first ")" $str ]

    set str [string range $str [expr $idx1 + 1] [expr $idx2 - 1]]
    
    debug "Gateway used is $str"

    set used_gw $str

    if { $used_gw != $gw } {
        return -code 1 "Incorrect gateway used ($used_gw) to $remoteAddr"
    }

    debug "Gateway $gw used to $remoteAddr"

    return -code 0 "OK"
}

#
# Executes a command on a remote host and returns resulting string
#
proc tcr_rexec { ip cmd } {
    debug "Running $cmd on $ip"

    if { [catch {set str [exec ssh root@$ip "$cmd"] } res ] } {
        return -code 1 "$res"
    }    

    return -code 0 "$str"
}

