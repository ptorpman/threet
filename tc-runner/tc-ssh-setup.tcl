#!/usr/bin/expect
#-------------------------------------------------------------------------------
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
#-------------------------------------------------------------------------------
# This program is used to copy a user's SSH identity to a remote host in order
# to be able to run programs on that host without the need for entering a 
# password.
#-------------------------------------------------------------------------------

global pass

log_user 0

set ip   [lindex $::argv 0]
set user [lindex $::argv 1]
set pass [lindex $::argv 2]

puts "* Copying SSH identity for $user to $ip ..."

spawn /usr/bin/ssh-copy-id $user@$ip
expect {
    "assword: " {
        send "$pass\n"
        expect {
            "again."     { exit 1 }
            "expecting." { }
            timeout      { exit 1 }
        }
    }
    "(yes/no)? " {
        send "yes\n"
        expect {
            "assword: " {
                send "$password\n"
                expect {
                    "again."     { exit 1 }
                    "expecting." { }
                    timeout      { exit 1 }
                }
            }
        }
    }
}
 
exit 0
EOT
