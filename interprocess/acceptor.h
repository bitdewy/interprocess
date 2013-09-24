//  Copyright 2013, bitdewy@gmail.com
//  Distributed under the Boost Software License, Version 1.0.
//  You may obtain a copy of the License at
//
//  http://www.boost.org/LICENSE_1_0.txt

#ifndef INTERPROCESS_ACCEPTOR_H_
#define INTERPROCESS_ACCEPTOR_H_

#include <windows.h>
#include <thread>
#include <string>
#include "interprocess/types.h"

namespace interprocess {

class Acceptor : public noncopyable {
 public:
  typedef std::function<void(HANDLE, HANDLE)> NewConnectionCallback;

  explicit Acceptor(const std::string& endpoint);
  ~Acceptor();
  void Listen();
  void Stop();
  void SetNewConnectionCallback(const NewConnectionCallback& cb);
  void SetExceptionCallback(const ExceptionCallback& cb);
  void MoveIOFunctionToAlertableThread(const std::function<void()>& cb);

 private:
  void ListenInThread();
  bool CreateConnectInstance();

  const std::string pipe_name_;
  std::thread listen_thread_;
  HANDLE next_pipe_;
  HANDLE close_event_;
  OVERLAPPED connect_overlap_;
  NewConnectionCallback new_connection_callback_;
  ExceptionCallback exception_callback_;
  std::function<void()> async_io_callback_;
};

}  // namespace interprocess

#endif  // INTERPROCESS_ACCEPTOR_H_
