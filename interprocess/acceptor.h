//  Copyright 2013, bitdewy@gmail.com
//  Distributed under the Boost Software License, Version 1.0.
//  You may obtain a copy of the License at
//
//  http://www.boost.org/LICENSE_1_0.txt

#ifndef INTERPROCESS_ACCEPTOR_H_
#define INTERPROCESS_ACCEPTOR_H_

#include <windows.h>
#include <thread>
#include <map>
#include <string>
#include "interprocess/types.h"

namespace interprocess {

class Acceptor : public noncopyable {
 public:
  explicit Acceptor(const std::string& endpoint);
  ~Acceptor();
  void Listen();
  void Stop();
  void SetNewConnectionCallback(const NewConnectionCallback& cb);
  void SetExceptionCallback(const ExceptionCallback& cb);
  void MoveAsyncIOFunctionToAlertableThread(const std::function<void()>& cb);
  void MoveWaitResponseIOFunctionToAlertableThread(
    const std::function<void()>& cb);

 private:
  void ListenInThread();
  bool CreateConnectInstance();
  bool Pendding(int err);

  const std::string pipe_name_;
  std::thread listen_thread_;
  std::map<int, std::function<bool()>> pendding_function_map_;
  HANDLE next_pipe_;
  HANDLE close_event_;
  OVERLAPPED connect_overlap_;
  NewConnectionCallback new_connection_callback_;
  ExceptionCallback exception_callback_;
  std::function<void()> async_io_callback_;
  std::function<void()> async_wait_io_callback_;
};

}  // namespace interprocess

#endif  // INTERPROCESS_ACCEPTOR_H_
