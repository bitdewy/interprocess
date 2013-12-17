//  Copyright 2013, bitdewy@gmail.com
//  Distributed under the Boost Software License, Version 1.0.
//  You may obtain a copy of the License at
//
//  http://www.boost.org/LICENSE_1_0.txt

#ifndef INTERPROCESS_CONNECTOR_H_
#define INTERPROCESS_CONNECTOR_H_

#include <thread>
#include <string>
#include "interprocess/types.h"

namespace interprocess {

class Connector : public noncopyable {
 public:
  explicit Connector(const std::string& endpoint);
  ~Connector();
  void Start();
  void Stop();
  void SetNewConnectionCallback(const NewConnectionCallback& cb);
  void SetExceptionCallback(const ExceptionCallback& cb);
  void MoveIOFunctionToAlertableThread(const std::function<void()>& cb);

 private:
  HANDLE CreateConnectionInstance();
  void ConnectInThread();

  std::string pipe_name_;
  std::thread connect_thread_;
  HANDLE post_event_;
  HANDLE send_event_;
  HANDLE close_event_;
  NewConnectionCallback new_connection_callback_;
  ExceptionCallback exception_callback_;
  std::function<void()> async_io_callback_;
};

}  // namespace interprocess

#endif  // INTERPROCESS_CONNECTOR_H_
