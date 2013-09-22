//  Copyright 2013, bitdewy@gmail.com
//  Distributed under the Boost Software License, Version 1.0.
//  You may obtain a copy of the License at
//
//  http://www.boost.org/LICENSE_1_0.txt

#ifndef INTERPROCESS_CLIENT_H_
#define INTERPROCESS_CLIENT_H_

#include <memory>
#include <string>
#include "interprocess/types.h"

namespace interprocess {

class Client : public noncopyable {
 public:
  explicit Client(const std::string& name);
  ~Client();
  void Connect(const std::string& server_name);
  std::string Name() const;
  ConnectionPtr Connection();
  void SetMessageCallback(const MessageCallback& callback);
  void SetExceptionCallback(const ExceptionCallback& cb);
  void Stop();

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace interprocess

#endif  // INTERPROCESS_CLIENT_H_
