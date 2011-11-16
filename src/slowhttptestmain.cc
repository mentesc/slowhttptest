/*****************************************************************************
*  Copyright 2011 Sergey Shekya,n Victor Agababov
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
* *****************************************************************************/

/*****
 * Author: Sergey Shekyan shekyan@gmail.com
 *         Victor Agababov vagababov@gmail.com
 *
 * Slow HTTP attack vulnerability test tool
 *  http://code.google.com/p/slowhttptest/
 *****/
#include "config.h"
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <memory>
#include <cctype>

#include "slowlog.h"
#include "slowhttptest.h"

#define DEFAULT_URL "http://localhost/"

static void usage() {
  printf(
      "\n%s %s, a tool to test for slow HTTP "
      "DoS vulnerabilities.\n"
      "Usage:\n"
      "slowtest [-a <range start>] [-b <range limit>]\n"
      "[-c <number of connections>] [-<H|B|R|X>]\n"
      "[-g <generate statistics>]\n"
      "[-i <interval in seconds>] [-l <test duration in seconds>]\n"
      "[-o <output file path and/or name>]\n"
      "[-p <timeout for probe connection>]\n"
      "[-r <connections per second>]\n"
      "[-s <value of Content-Length header>] [-t <verb>]\n"
      "[-u <URL>] [-v <verbosity level>] \n"
      "[-w <recv buffer limit>] [-x <max length of follow up data>]\n"
      "Options:\n\t"
      "-a start,        left boundary of range in range header, default: 5\n\t"
      "-b bytes,        limit for range header right boundary values, default: 2000\n\t"
      "-c connections,  target number of connections, default: 50\n\t"
      "-h               display this help and exit\n\t"
      "-H, -B, -R or X  specify test mode (slow headers,body, range or read),\n\t"
      "                 default: headers\n\t"
      "-g,              generate statistics with socket state changes,\n\t"
      "                 default: off\n\t"
      "-i seconds,      interval between followup data in seconds, default: 10\n\t"
      "-l seconds,      target test length in seconds, default: 240\n\t"
      "-o file,         save statistics output in file.html and file.csv,\n\t"
      "                 -g must be specified to use this option\n\t"
      "-p seconds,      timeout to wait for HTTP response on probe connection,\n\t"
      "                 after which server is considered inaccessible,\n\t"
      "                 default: 5\n\t"
      "-r num,          connection rate (connections per seconds), default: 50\n\t"
      "-s bytes,        value of Content-Length header if needed, default: 4096\n\t"
      "-t verb          verb to use in request,\n\t"
      "                 defalut to GET for slow headers and POST for slow body)\n\t"
      "-u URL,          absolute URL of target, default: http://localhost/\n\t"
      "-v level,        verbosity level 0-4: Fatal, Info, Error, Warning, Debug\n\t"
      "                 default: 1 - Info\n\t"
      "-w size,         upper limit of socket receive buffer to use\n\t"
      "                 with slow read (-X) test. Test uses random value for every connection\n\t"
      "                 picked between 2 and the limit, min: 1, default: 2\n\t"
      "-x bytes,        max length of each randomized name/value pair of\n\t"
      "                 followup data per tick, e.g. -x 2 generates\n\t"
      "                 X-xx: xx for header or &xx=xx for body, where x\n\t"
      "                 is random character, default: 32\n"
      , PACKAGE
      , VERSION
      );
}

// global flag to indicite if we need to run
int g_running = true;

void int_handler(int param) {
  g_running = false;  
}

using slowhttptest::slowlog_init;
using slowhttptest::slowlog;
using slowhttptest::SlowHTTPTest;
using slowhttptest::SlowTestType;

int main(int argc, char **argv) {

  if (argc < 1) {
    usage();
    return -1;
  }
  char url[1024] = { 0 };
  char path[1024] = { 0 };
  char verb[16] = { 0 };
  // default vaules
  int conn_cnt            = 50;
  int content_length      = 4096;
  int duration            = 240;
  int interval            = 10;
  int max_random_data_len = 32;
  int probe_interval      = 5;
  int range_start         = 5;
  int range_limit         = 2000;
  int rate                = 50;
  int read_interval       = 5;
  int debug_level         = LOG_INFO;
  bool  need_stats        = false;
  int window_size_limit    = 2;
  SlowTestType type = slowhttptest::eHeader;
  long tmp;
  char o;
  while((o = getopt(argc, argv, ":HBRXga:b:c:i:l:o:p:r:s:t:u:v:w:x:")) != -1) {
    switch (o) {
      case 'a':
        tmp = strtol(optarg, 0, 10);
        if(tmp && tmp <= 65539) {
          range_start = tmp;
        } else {
          usage();
          return -1;
        }
        break;
      case 'b':
        tmp = strtol(optarg, 0, 10);
        if(tmp && tmp <= 524288) {
          range_limit = tmp;
        } else {
          usage();
          return -1;
        }
        break;
      case 'c':
        tmp = strtol(optarg, 0, 10);
        if(tmp && tmp <= 1024) {
          conn_cnt = static_cast<int>(tmp);
        } else {
          usage();
          return -1;
        }
        break;
      case 'h':
        usage();
        return 1;
        break;
      case 'H':
        type = slowhttptest::eHeader;
        break;
      case 'B':
        type = slowhttptest::ePost;
        break;
      case 'R':
        type = slowhttptest::eRange;
        break;
      case 'X':
        type = slowhttptest::eSlowRead;
        break;
      case 'g':
        need_stats = true;
        break;
      case 'i':
        tmp = strtol(optarg, 0, 10);
        if(tmp && tmp <= INT_MAX) {
          interval = static_cast<int>(tmp);
        } else {
          usage();
          return -1;
        }
        break;
      case 'l':
        tmp = strtol(optarg, 0, 10);
        if(tmp && tmp <= INT_MAX) {
          duration = static_cast<int>(tmp);
        } else {
          usage();
          return -1;
        }
        break;
      case 'o':
        strncpy(path, optarg, 1023);
        break;
      case 'p':
        tmp = strtol(optarg, 0, 10);
        if(tmp && tmp <= INT_MAX) {
          probe_interval = static_cast<int>(tmp);
        } else {
          usage();
          return -1;
        }
        break;
      case 'r':
        tmp = strtol(optarg, 0, 10);
        if(tmp && tmp <= INT_MAX) {
          rate = static_cast<int>(tmp);
        } else {
          usage();
          return -1;
        }
        break;
      case 's':
        tmp = strtol(optarg, 0, 10);
        if(tmp && tmp <= INT_MAX) {
          content_length = static_cast<int>(tmp);
        } else {
          usage();
          return -1;
        }
        break;
      case 't':
        strncpy(verb, optarg, 15);
        break;
      case 'u':
        strncpy(url, optarg, 1023);
        break;
      case 'v':
        tmp = strtol(optarg, 0, 10);
        if(0 <= tmp && tmp <= 4) {
          debug_level = static_cast<int>(tmp);
        } else {
          debug_level = LOG_FATAL;
        }
        break;
      case 'w':
        tmp = strtol(optarg, 0, 10);
        if(1 <= tmp && tmp <= INT_MAX) {
          window_size_limit = tmp;
        } else {
          usage();
          return -1;
        }
        break;
      case 'x':
        tmp = strtol(optarg, 0, 10);
        if(tmp && tmp <= INT_MAX) {
          max_random_data_len = static_cast<int>(tmp);
          if(max_random_data_len < 2) max_random_data_len = 2;
        } else {
          usage();
          return -1;
        }
        break;
      case '?':
        printf("Illegal option -%c\n", optopt);
        usage();
        return -1;
        break;
      default:
        printf("Option -%c requires an argument.\n", optopt);
        usage();
        return -1;
    }
  }
  if(!strlen(url)) {
    strncpy(url, DEFAULT_URL, sizeof(DEFAULT_URL));
  }
  signal(SIGPIPE, SIG_IGN);
  signal(SIGINT, &int_handler);
  slowlog_init(debug_level, NULL);
  std::auto_ptr<SlowHTTPTest> slow_test(
      new SlowHTTPTest(rate, duration, interval, conn_cnt, 
      max_random_data_len, content_length, type, need_stats,
      probe_interval, range_start, range_limit,
      read_interval, window_size_limit));
  if(!slow_test->init(url, verb, path)) {
    slowlog(LOG_FATAL, "%s: error setting up slow HTTP test\n", __FUNCTION__);
    return -1;
  } else if(!slow_test->run_test()) {
    slowlog(LOG_FATAL, "%s: error running slow HTTP test\n", __FUNCTION__);
    return -1;
  }
  slow_test->report_final();
  return 0;
}
