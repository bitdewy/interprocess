//  Copyright 2013, bitdewy@gmail.com
//  Distributed under the Boost Software License, Version 1.0.
//  You may obtain a copy of the License at
//
//  http://www.boost.org/LICENSE_1_0.txt

#include "interprocess/client.h"
#include <windows.h>
#include <memory>
#include <string>
#include <utility>
#include "interprocess/connector.h"
#include "interprocess/connection.h"

namespace interprocess {

class Client::Impl {
 public:
  explicit Impl(const std::string& name);
  ~Impl();
  void Connect(const std::string& server_name);
  std::string Name() const;
  ConnectionPtr Connection();
  void SetMessageCallback(const MessageCallback& cb);
  void Stop();

 private:
  void NewConnection(HANDLE pipe, HANDLE write_event);
  void ResetConnection(const ConnectionPtr& conn);
  void SendInAlertableThread();

  ConnectionPtr conn_;
  std::unique_ptr<Connector> connector_;
  std::string name_;
  MessageCallback message_callback_;
};

// real implement of Client

Client::Impl::Impl(const std::string& name)
  : name_(name) {}

Client::Impl::~Impl() {}

void Client::Impl::Connect(const std::string& server_name) {
  using std::placeholders::_1;
  using std::placeholders::_2;
  connector_.reset(new Connector(server_name));
  connector_->SetNewConnectionCallback(
    std::bind(&Client::Impl::NewConnection, this, _1, _2));
  connector_->MoveIOFunctionToAlertableThread(
    std::bind(&Client::Impl::SendInAlertableThread, this));
  connector_->Start();
}

std::string Client::Impl::Name() const {
  return name_;
}

ConnectionPtr Client::Impl::Connection() {
  return conn_;
}

void Client::Impl::SetMessageCallback(const MessageCallback& cb) {
  message_callback_ = cb;
}

void Client::Impl::Stop() {
  connector_->Stop();
}

void Client::Impl::NewConnection(HANDLE pipe, HANDLE write_event) {
  using std::placeholders::_1;
  using interprocess::Connection;
  auto name = name_.append("#").append(
    std::to_string(reinterpret_cast<int32_t>(pipe)));
  conn_ = std::make_shared<Connection>(name, pipe, write_event);
  conn_->SetCloseCallback(
    std::bind(&Client::Impl::ResetConnection, this, _1));
  ConnectionAttorney::SetMessageCallback(conn_, message_callback_);
}

void Client::Impl::ResetConnection(const ConnectionPtr& conn) {
  conn_.reset();
}

void Client::Impl::SendInAlertableThread() {
  if (conn_) {
    ConnectionAttorney::AsyncWrite(conn_);
  }
}

// Client wrapper

Client::Client(const std::string& name)
  : impl_(new Impl(name)) {}

Client::~Client() {}

void Client::Connect(const std::string& server_name) {
  impl_->Connect(server_name);
}

std::string Client::Name() const {
  return impl_->Name();
}

ConnectionPtr Client::Connection() {
  return impl_->Connection();
}

void Client::SetMessageCallback(const MessageCallback& cb) {
  impl_->SetMessageCallback(cb);
}

void Client::Stop() {
  impl_->Stop();
}

}  // namespace interprocess
