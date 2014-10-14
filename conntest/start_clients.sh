#!/bin/bash
#  Copyright (c) 2012  Peter R. Torpman (peter at torpman dot se)
# 
#  This file is part of Torpman's Test Tools 
#         https://github.com/ptorpman/threet
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
# This script is a way of starting X number of clients that each have 64
# threads/connections towards a server.
# Throughput is reported to an aggregator at port 10000.
#

printUsage() {
    echo "Usage: start_clients <num> <serverip> <base port> <throughput> <num_measurements>"
}


if [ $# -lt 5 ]; then 
    printUsage
    exit 1
fi

dname=`dirname $0`

AGGREGATOR="$dname/aggregator"
CONNTEST="$dname/conntest"


num_clients=$1
server_ip="$2"
base_port=$3
tput_total=$4
num_measure=$5

tput_per_client=`expr $tput_total / $num_clients`

# Start aggregator
$AGGREGATOR -n $num_clients -p 30000 -m $num_measure &

# Start clients
id=0
while [ $id -lt $num_clients ]; do
    sport=`expr $base_port + $id`
    $CONNTEST -c -id $id -l $server_ip -t $tput_per_client -n 64 -p $sport -a 30000 &> /dev/null &
    id=`expr $id + 1`;
done

tot_con=`expr $num_clients \* 64`

echo "* Started $num_clients with $tot_con connections for throughput: $tput_total"

