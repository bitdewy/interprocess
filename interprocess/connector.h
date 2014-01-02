//  Copyright 2014, bitdewy@gmail.com
//  Distributed under the Boost Software License, Version 1.0.
//  You may obtain a copy of the License at
//
//  http://www.boost.org/LICENSE_1_0.txt

#ifndef INTERPROCESS_CONNECTOR_H_
#define INTERPROCESS_CONNECTOR_H_

#include <string>
#include <thread>
#include "interprocess/types.h"

namespace interprocess {

class Connector {
 public:
  explicit Connector(const std::string& endpoint);
  Connector(const Connector&) = delete;
  Connector& operator=(const Connector&) = delete;
  ~Connector();
  void Start();
  void Stop();
  void SetNewConnectionCallback(const NewConnectionCallback& cb);
  void SetExceptionCallback(const ExceptionCallback& cb);
  void MoveAsyncIOFunctionToAlertableThread(const std::function<void()>& cb);
  void MoveWaitResponseIOFunctionToAlertableThread(
    const std::function<void()>& cb);

 private:
  HANDLE CreateConnectionInstance();
  void ConnectInThread();

  std::string pipe_name_;
  std::thread connect_thread_;
  HANDLE close_event_;
  NewConnectionCallback new_connection_callback_;
  ExceptionCallback exception_callback_;
  std::function<void()> async_io_callback_;
  std::function<void()> async_wait_io_callback_;
};

}  // namespace interprocess

#endif  // INTERPROCESS_CONNECTOR_H_
