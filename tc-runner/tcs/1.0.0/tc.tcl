# 
# This is a sample tc-runner configuration file.
# This test case sends UDP packages from one host, which are detected by tshark on the other host.

# Start a tool (tshark) on a host
run_exe hostB 1.log "tshark -i eth0 -a duration:5 -R \"(eth contains mause) and udp.srcport == 24100 and udp.dstport == 24101 and not icmp\""

# Start mausezahn on an other host
run_exe hostA 2.log "mz -A $TCR(ip,Aint) -B $TCR(ip,Cwan1) -t udp \"sp=24100,dp=24101\" -P mause -c 5"

# Stop current processes
stop_all_procs

# Perform validation by examining logs e.g
grep_check $TCR(logs)/1.log 5 "Source port: 24100  Destination port: 24101"

# Start a tool (tshark) on a host
run_exe hostB 3.log "tshark -i eth0 -a duration:5 -R \"(eth contains mause) and udp.srcport == 24101 and udp.dstport == 24100 and not icmp\""

# Start mausezahn on an other host
run_exe hostB  4.log "mz -A $TCR(ip,Bint) -B $TCR(ip,Aint) -t udp sp=24101,dp=24100 -P mause -c 5"

# Stop current processes
stop_all_procs

# Perform validation by examining logs e.g
grep_check $TCR(logs)/3.log 5 "Source port: 24101  Destination port: 24100"




