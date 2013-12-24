//  Copyright 2013, bitdewy@gmail.com
//  Distributed under the Boost Software License, Version 1.0.
//  You may obtain a copy of the License at
//
//  http://www.boost.org/LICENSE_1_0.txt

#ifndef INTERPROCESS_SERVER_H_
#define INTERPROCESS_SERVER_H_

#include <memory>
#include <string>
#include "interprocess/types.h"

namespace interprocess {

class Server {
 public:
  explicit Server(const std::string& endpoint);
  Server(const Server&) = delete;
  Server(Server&& other);
  Server& operator=(const Server&) = delete;
  Server& operator=(Server&& other);
  ~Server();
  void swap(Server& other);
  void Listen();
  void Stop();
  void SetMessageCallback(const MessageCallback& cb);
  void SetExceptionCallback(const ExceptionCallback& cb);
  void Broadcast(const std::string& message);
  void CloseConnection(const std::string& name);

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace interprocess

#endif  // INTERPROCESS_SERVER_H_
