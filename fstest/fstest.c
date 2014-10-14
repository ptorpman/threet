/* Copyright (c) 2012  Peter R. Torpman (peter at torpman dot se) 
 
   This file is part of Torpman's Test Tools  
   https://github.com/ptorpman/threet

   Torpman's Test Tools is free software; you can redistribute it and/or modify 
   it under the terms of the GNU General Public License as published by 
   the Free Software Foundation; either version 3 of the License, or 
   (at your option) any later version. 

   Torpman's Test Tools is distributed in the hope that it will  
   be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of 
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
   GNU General Public License for more details. 

   You should have received a copy of the GNU General Public License 
   along with this program.  If not, see <http://www.gnu.or/licenses/>. 
*/

/*============================================================================*/
/* INCLUDES                                                                   */
/*============================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>
#include <unistd.h>

#include "fstest.h"



/*============================================================================*/
/* FUNCTION DECLARATIONS                                                      */
/*============================================================================*/
static void print_usage(void);
static void print_version(void);
static void run_read_test(void);
static void run_write_test(void);
static uint32_t get_size(char* str);


/*============================================================================*/
/* VARIABLES                                                                  */
/*============================================================================*/
fs_testtype_t gTestType  = TEST_TYPE_READ;
uint32_t      gFileSize  = 1024; /* File size to use */
uint32_t      gDuration  = 10;  /* Duration in seconds */
uint32_t      gBlockSize = 1;   /* Read/Write block size */
char          gTestFile[0xFF];  /* Test file name */


/*============================================================================*/
/* FUNCTION DEFINITIONS                                                       */
/*============================================================================*/

int
main(int argc, char** argv)
{
  /* Default file name */
  strcpy(gTestFile, "/tmp/fstest.tmp");

  
  /* Check parameters */
  int i = 1;
  int val = 0;

  if (argc < 2) {
    print_usage();
    return 0;
  }
  
  while (i < argc) {

    if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
      print_usage();
      return 0;
    }
    else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version")) {
      print_version();
      return 0;
    }
    else if (!strcmp(argv[i], "-r")) {
      gTestType = TEST_TYPE_READ;
      i++;
      continue;
    }
    else if (!strcmp(argv[i], "-w")) {
      gTestType = TEST_TYPE_WRITE;
      i++;
      continue;
    }
    else if (!strcmp(argv[i], "-s")) {
      if (argv[i + 1] == NULL) {
        print_usage();
        return 1;
      }

      val = get_size(argv[i+1]);

      if (val <= 0) {
        fprintf(stderr, "ERROR: Incorrect size value '%s'\n", argv[i+1]);
        return 1;
      }

      gFileSize = (uint32_t) val;
     
      i += 2;
    }
    else if (!strcmp(argv[i], "-b")) {
      if (argv[i + 1] == NULL) {
        print_usage();
        return 1;
      }

      val = get_size(argv[i+1]);

      if (val <= 0) {
        fprintf(stderr, "ERROR: Incorrect block size value '%s'\n", argv[i+1]);
        return 1;
      }

      gBlockSize = (uint32_t) val;
     
      i += 2;
    }
    else if (!strcmp(argv[i], "-t")) {
      if (argv[i + 1] == NULL) {
        print_usage();
        return 1;
      }
      
      if (sscanf(argv[i+1], "%u", &val) != 1) {
        fprintf(stderr, "ERROR: Incorrect duration value '%s'\n", argv[i+1]);
        return 1;
      }
      
      
      gDuration = val;
      i += 2;
    }

    else if (!strcmp(argv[i], "-f")) {
      if (argv[i + 1] == NULL) {
        print_usage();
        return 1;
      }
      
      strcpy(gTestFile, argv[i+1]);

      /* Do some file checks */
      

      if (access(gTestFile, F_OK) == 0) {
        fprintf(stderr, "ERROR: File exists (%s)\n", gTestFile);
        return 1;
      }
      
      i += 2;
    }
    
    
    else {
      fprintf(stderr, "Bad parameter! (%s)\n", argv[i]);
      print_usage();
      return 1;
    }
  }
  
  fprintf(stderr, "* Using file size %u bytes (%ukB %uMB %uGB). Duration: %u seconds.\n",
          gFileSize, gFileSize / 1024, gFileSize / (1024 * 1024),
          gFileSize / (1024 * 1024 * 1024), gDuration);

  switch (gTestType) {
  case TEST_TYPE_READ:
    run_read_test();
    break;
  case TEST_TYPE_WRITE:
    run_write_test();
    break;
  }

  
  
  return 0;
}



static void
print_usage(void)
{
    fprintf(stderr,
            "Usage: fstest [options]\n"
            "  Options:\n"
            "  -r         Read test\n"
            "  -w         Write test\n"
            "  -d         Delete test\n"
            "  -s <size>  Size in bytes. Can specify using k, M and G.\n"
            "  -b <size>  Number of bytes to read/write per operation. "
            "Can specify using k, M and G.\n"
            "  -t <secs>  Duration in seconds. \n"
            "  -f <name>  File name (including path)\n"
            "  -v         Display version information\n"
            "  -h         Display this text\n\n"
            "For bug reporting and suggestions, mail peter@torpman.se\n");
}

static void
print_version(void)
{
    fprintf(stderr,
            "fstest (Torpman's Test Tools) 0.1\n"
            "Copyright (C) 2012 Peter R. Torpman (peter@torpman.se)\n"
            "This is free software.  \nYou may redistribute copies of it under "
            "the terms of the GNU General Public License \n"
            "<http://www.gnu.org/licenses/gpl.html>.\n"
            "There is NO WARRANTY, to the extent permitted by law.\n");
}

static uint32_t
get_size(char* str)
{
  uint32_t val = 0;
  
  if (strchr(str, 'k')) {
    /* Kilobyte marker used */
    if (sscanf(str, "%u%*c", &val) != 1) {
      fprintf(stderr, "ERROR: Incorrect value '%s'\n", str);
      return -1;
    }

    return (val * 1024);
  }
  else if (strchr(str, 'M')) {
    /* Megabyte marker used */
    if (sscanf(str, "%u%*c", &val) != 1) {
      fprintf(stderr, "ERROR: Incorrect value '%s'\n", str);
      return -1;
    }
    return (val * 1024 * 1024);
  }
  else if (strchr(str, 'G')) {
    /* Gigabyte marker used */
    int res = sscanf(str, "%uG", &val);
    if (res != 1) {
      fprintf(stderr, "ERROR: Incorrect value '%s' (%d)\n", str, res);
      return -1;
    }
    return (val * 1024 * 1024 * 1024);
  }
  else {
    if (sscanf(str, "%u", &val) != 1) {
      fprintf(stderr, "ERROR: Incorrect value '%s'\n", str);
      return -1;
    }

    return val;
  }
}

static void
run_read_test(void)
{
  /* First, create a file with the correct size. */
  char cmd[0xFF];

  sprintf(cmd, "dd if=/dev/zero of=%s bs=%u count=1", gTestFile, gFileSize);
   
  fprintf(stderr, "* Creating file of %u bytes size (%s) ...", gFileSize, cmd);

  if (system(cmd) == -1) {
    fprintf(stderr, "ERROR: Could not create file\n");
    return;
  }
  
  fprintf(stderr, "done.\n");

  /* Then, set the start time */
  struct timeval tStart;
  struct timeval tCurr;
  gettimeofday(&tStart, NULL);
  
  char* buff = (char*) malloc(gBlockSize);

  int fileReadTimes = 0;
  uint32_t bytesRead = 0;
  
  /* Then start reading the file... */
  while (1) {                   /* While time is left */

    FILE* fp = fopen(gTestFile, "r");

    
    while (!feof(fp)) {         /* While file is not at EOF */
      int res = fread(buff, 1, gBlockSize, fp);

      if (res <= 0) {
        break;
      }
    }

    fclose(fp);

    fileReadTimes++;
    bytesRead += gFileSize;
    
    gettimeofday(&tCurr, NULL);
    if (((uint32_t)(tCurr.tv_sec - tStart.tv_sec)) >= gDuration) {
      /* Time is up! */
      break;
    }
  }

  fprintf(stderr,
          "* File read %u kilotimes in %d seconds (%.2f kB/s %.2f MB/s). "
          "Bytes read: %u (%u kB %u MB) \n",
          fileReadTimes / 1024,
          (uint32_t)(tCurr.tv_sec - tStart.tv_sec),
          (double)((bytesRead / 1024) / (double)(tCurr.tv_sec - tStart.tv_sec)),
          (double)((bytesRead / (1024*1024)) / (double)(tCurr.tv_sec-tStart.tv_sec)),
          bytesRead, bytesRead / 1024, bytesRead / (1024*1024));

  
  /* Finally remove the created file. */
  if (remove(gTestFile) != 0) {
    fprintf(stderr, "ERROR: Could not remove file\n");
  }
}


static void
run_write_test(void)
{
  /* Do some initial checks */
  if (gFileSize < gBlockSize) {
    fprintf(stderr, "ERROR: Block size bigger than file size!\n");
    return;
  }

  /* Then, set the start time */
  struct timeval tStart;
  struct timeval tCurr;
  gettimeofday(&tStart, NULL);
  
  char* buff = (char*) malloc(gBlockSize);

  /* Initialize buffer with text... */
  memset(buff, 'a', gBlockSize);

  int fileWrittenTimes = 0;
  uint32_t bytesWritten = 0;
  int numToWrite = gBlockSize;
  uint32_t left = 0;
  
  
  /* Then start reading the file... */
  while (1) {                   /* While time is left */
    left       = gFileSize;
    numToWrite = gBlockSize;
    
  /* First, open up a file handle to use */
    FILE* fp = fopen(gTestFile, "w+");
    
    if (!fp) {
      fprintf(stderr, "ERROR: Could not open file!\n");
      return;
    }
    
    while (1) {
      int res = fwrite(buff, 1, numToWrite, fp);

      if (res > 0) {
        bytesWritten += res;
    
        left -= res;
        
        if (left > gBlockSize) {
          numToWrite = gBlockSize;
        }
        else {
          numToWrite = left;
        }

        if (numToWrite <= 0) {
          bytesWritten += gFileSize;
          break;
        }
      }
      else {
        break;
      }
    }

    fclose(fp);
    
    fileWrittenTimes++;
    
    gettimeofday(&tCurr, NULL);
    if (((uint32_t)(tCurr.tv_sec - tStart.tv_sec)) >= gDuration) {
      /* Time is up! */
      break;
    }
  }

  fprintf(stderr,
          "* File written %u kilotimes in %d seconds (%.2f kB/s %.2f MB/s). "
          "Bytes written: %u (%u kB %u MB) \n",
          fileWrittenTimes / 1024,
          (uint32_t)(tCurr.tv_sec - tStart.tv_sec),
          (double)((bytesWritten / 1024) / (double)(tCurr.tv_sec - tStart.tv_sec)),
          (double)((bytesWritten / (1024*1024)) / (double)(tCurr.tv_sec-tStart.tv_sec)),
          bytesWritten, bytesWritten / 1024, bytesWritten / (1024*1024));

 
  /* Finally remove the created file. */
  if (remove(gTestFile) != 0) {
    fprintf(stderr, "ERROR: Could not remove file\n");
  }
}



