This program is used for testing file system thoughput. 

Features included are:
- Read as many files per second as possible
- Write as many files per second  as possible

It is possible to specify the size of the files to be read, written and deleted.

Usage:
  ftest [OPTIONS]

  Options:
  -r	     Read test
  -w         Write test
  -s <size>  Size in bytes. Can be specified using k, M and G.
  -b <size>  Block size in bytes. Can be specified using k, M and G.
  -t <secs>  Duration in seconds. 
  

Examples:

  Read as many 1kB files as possible for a minute:

       fstest -r -s 1k -b 1k -t 60 

  Write as many 1kB files as possible for a minute:

       fstest -w -s 1k -b 1k -t 60


