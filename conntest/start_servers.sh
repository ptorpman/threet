#!/bin/bash
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

#
# This script is a way of starting X number of servers that each have 64
# threads/connections towards a number of clients.
#

printUsage() {
    echo "Usage: start_servers <num> <base port> <num_measurements>"
}


if [ $# -lt 3 ]; then 
    printUsage
    exit 1
fi

dname=`dirname $0`

AGGREGATOR="$dname/aggregator"
CONNTEST="$dname/conntest"

num_clients=$1
base_port=$2
num_measure=$3


# Start aggregator
$AGGREGATOR -n $num_clients -p 30001 -m $num_measure &

# Start servers

id=0
while [ $id -lt $num_clients ]; do
    sport=`expr $base_port + $id`
    $CONNTEST -id $id -s  -t 1 -n 64 -p $sport -a 30001 &> /dev/null &
    id=`expr $id + 1`;
done

tot_con=`expr $num_clients \* 64`

echo "* Started $num_clients servers for $tot_con connections..."




