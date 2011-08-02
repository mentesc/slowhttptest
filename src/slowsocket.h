/*****************************************************************************
*  Copyright 2011 Sergey Shekyan
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
 * Author: Sergey Shekyan sshekyan@qualys.com
 *
 * Slow HTTP attack  vulnerability test tool
 *  http://code.google.com/p/slowhttptest/
 *
 *  class SlowSocket is a wrapper around TCP socket structure.
 *  class gives unified access to main socket operations no matter 
 *  if it is plain or SSL connection.
 *****/

#ifndef _SLOWSOCKET_H_
#define _SLOWSOCKET_H_

#include <sys/time.h>

#include <string>
#include <openssl/ssl.h>

struct addrinfo;

namespace slowhttptest {
class Url;

enum SendType {
  eInitialSend = 0, eFollowUpSend
};

enum SocketState {
  eInit = 0, eError, eConnecting, eConnected, eClosed
};

class SlowSocket {
 public:
  SlowSocket();
  ~SlowSocket();
  bool isEmpty() {
    return -1 == sockfd_ && !ssl_;
  }
  const bool is_connect_initiated() const {return connect_initiated_;};
  void close();

  bool init(addrinfo* addr, const Url* url, int& maxfd,
      int followups_to_send);
  int recv_slow(void* buf, size_t len);
  int send_slow(const void* msg, size_t len, const SendType type =
      eInitialSend);
  const int get_sockfd() const {
    return sockfd_;
  }
  const int get_requests_to_send() const {
    return requests_to_send_;
  }
  const int get_followups_to_send() const {
    return followups_to_send_;
  }
  
  const int get_last_followup_timing() const {
    return last_followup_timing_;
  }

  void set_last_followup_timing(int timing) {
    last_followup_timing_ = timing;
  }

  void set_connected(const timeval* t) {
    connected_in_millisecs_ = t->tv_sec * 1000;
    connected_in_millisecs_ += t->tv_usec / 1000;
  }

  const long get_connected() const {
    return connected_in_millisecs_ ;
  }

  void set_start(const timeval* t) {
    start_in_millisecs_ = t->tv_sec * 1000;
    start_in_millisecs_ += t->tv_usec / 1000;
  }

  const long get_start() const {
    return start_in_millisecs_ ;
  }

  void set_stop(const timeval* t) {
    stop_in_millisecs_ = t->tv_sec * 1000;
    stop_in_millisecs_ += t->tv_usec / 1000;
  }

  const long get_stop() const {
    return stop_in_millisecs_ ;
  }

  void set_state(SocketState state) {
    state_ = state;
  }

  const SocketState& get_state() const {
    return state_;
  }

 private:
  bool connect_plain(addrinfo* addr);
  bool connect_ssl(addrinfo* addr);
  int set_nonblocking();
  
  int connect_initiated_;
  int sockfd_;
  int requests_to_send_;
  int followups_to_send_;
  int last_followup_timing_;
  int offset_;
  SSL* ssl_;
  const void* buf_;
  long start_in_millisecs_;
  long connected_in_millisecs_;
  long stop_in_millisecs_;
  SocketState state_;
};

}  // namespace slowhttptest
#endif
