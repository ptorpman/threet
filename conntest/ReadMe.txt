================================================================================

 #####                       #######                     
#     #  ####  #    # #    #    #    ######  ####  ##### 
#       #    # ##   # ##   #    #    #      #        #   
#       #    # # #  # # #  #    #    #####   ####    #   
#       #    # #  # # #  # #    #    #           #   #   
#     # #    # #   ## #   ##    #    #      #    #   #   
 #####   ####  #    # #    #    #    ######  ####    #   

           (c) 2012 Peter R. Torpman (peter at torpman dot se)
================================================================================

This program is used for testing thoughput over a number of concurrent TCP connections. 
The program is using threads - both in client and server mode - to enable the tester to
push as much as possible.

Sometimes the system sets boundaries for how many threads are possible to execute at the
same time. So, therefore it might be needed to start multiple instances of clients and
servers.

The Conntest tool includes an Aggregator that can be used to gather up information about
many running clients and servers. The total throughput and the average will be printed
by the Aggregator, while the clients and servers remain silent.

So, e.g it could be possible to start X number of clients, Y number of servers containing
Z number of threads. When using the aggregator there would only be two prinouts - one for
the server aggregator, and one for the client aggregator.


EXAMPLE 1:
----------
We want to test approximately 920 Mbps throughput over 128 concurrent connections.
In this case we have a powerful system that allows us to have 128 concurrent threads
per process, so we only need one client and one server.

   1. Start the aggregator for client side (we select 60 measurements):

   	  ./aggregator -n 1 -p 10000 -m 60

   2. Start the aggregator for server side:

   	  ./aggregator -n 1 -p 10001 -m 60

   3. Start the server:

      ./conntest -id 0 -s -p 7011 -t 1 -n 128 -a 10001

   4. Start the client:

      ./conntest -id 0 -c -p 7011 -t 920000 -n 128 -a 10000


EXAMPLE 2:
----------
We want to test approximately 920 Mbps throughput over 128 concurrent connections.
However, our system cannot handle more than 64 concurrent threads. So, then we 
aneed to start two servers and two clients.

   1. Start the aggregator for client side:

   	  ./aggregator -n 2 -p 10000

   2. Start the aggregator for server side:

   	  ./aggregator -n 2 -p 10001

   3. Start the servers:

      ./conntest -id 0 -s -p 7011 -t 1 -n 64 -a 10001
      ./conntest -id 1 -s -p 7011 -t 1 -n 64 -a 10001

   4. Start the clients:

      ./conntest -id 0 -c -p 7011 -t 460000 -n 64 -a 10000
      ./conntest -id 0 -c -p 7011 -t 460000 -n 64 -a 10000


    Please, note that the 920 Mbps are now divided among the two clients so that
    they each push 460 Mbps.

EXAMPLE 3:
----------
We want to test how much throughput we can get with different packet sizes.

   1. Start the aggregator for server side:

   	  ./aggregator -n 2 -p 10000

   2. Start the server:

      ./conntest -id 0 -s -p 7011 -t 1 -n 64 -a 10000

   3. Start the client(s):

      ./conntest -id 0 -c -p 7011 -t 460000 -n 64 -a 10000 -z 100


Usage: conntest [options]
  Options:
  -id <num>   Process ID.
  -s          Server mode
  -c          Client mode
  -a <port>   Aggregator port
  -n <num>    Number of connections.
  -t <kbps>   Throughput to aim for in kbps
  -l <hostip> Host name/IP address
  -p <port>   Server port number
  -z <size>   Packet size
  -v          Display version information
  -h          Display this text

Usage: aggregator [options]
  Options:
  -n <num>    Number of clients/servers to aggregate
  -p <port>   Port number to listen to
  -h          Display this text


