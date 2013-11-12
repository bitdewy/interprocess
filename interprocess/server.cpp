//  Copyright 2013, bitdewy@gmail.com
//  Distributed under the Boost Software License, Version 1.0.
//  You may obtain a copy of the License at
//
//  http://www.boost.org/LICENSE_1_0.txt

#include "interprocess/server.h"
#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include "interprocess/acceptor.h"
#include "interprocess/connection.h"

namespace interprocess {

class Server::Impl {
 public:
  typedef std::map<std::string, ConnectionPtr> ConnectionMap;
  explicit Impl(const std::string& endpoint);
  ~Impl();
  void Listen();
  void Stop();
  void SetMessageCallback(const MessageCallback& cb);
  void SetExceptionCallback(const ExceptionCallback& cb);
  void Broadcast(const std::string& message);
  void CloseConnection(const std::string& name);

 private:
  void NewConnection(HANDLE pipe, HANDLE write_event);
  void RemoveConnection(const ConnectionPtr& conn);
  void SendInAlertableThread();

  ConnectionMap connection_map_;
  std::unique_ptr<Acceptor> acceptor_;
  std::string name_;
  MessageCallback message_callback_;
  ExceptionCallback exception_callback_;
};

// real implement of Server

Server::Impl::Impl(const std::string& endpoint)
  : acceptor_(new Acceptor(endpoint)),
    name_(endpoint) {}

Server::Impl::~Impl() {}

void Server::Impl::Listen() {
  using std::placeholders::_1;
  using std::placeholders::_2;
  acceptor_->SetNewConnectionCallback(
    std::bind(&Server::Impl::NewConnection, this, _1, _2));
  acceptor_->SetExceptionCallback(exception_callback_);
  acceptor_->MoveIOFunctionToAlertableThread(
    std::bind(&Server::Impl::SendInAlertableThread, this));
  acceptor_->Listen();
}

void Server::Impl::Stop() {
  acceptor_->Stop();
}

void Server::Impl::SetMessageCallback(const MessageCallback& cb) {
  message_callback_ = cb;
}

void Server::Impl::SetExceptionCallback(const ExceptionCallback& cb) {
  exception_callback_ = cb;
}

void Server::Impl::Broadcast(const std::string& message) {
  typedef std::pair<std::string, ConnectionPtr> ConnectionMapItem;
  std::for_each(std::begin(connection_map_),
                std::end(connection_map_),
                [=](const ConnectionMapItem& pair) {
    pair.second->Send(message);
  });
}


void Server::Impl::CloseConnection(const std::string& name) {
  typedef std::pair<std::string, ConnectionPtr> ConnectionMapItem;
  auto it = std::find_if(std::begin(connection_map_),
                         std::end(connection_map_),
                         [=](const ConnectionMapItem& pair) {
    return name == pair.first;
  });
  if (it != std::end(connection_map_)) {
    it->second->Close();
  }
}

void Server::Impl::NewConnection(HANDLE pipe, HANDLE write_event) {
  using std::placeholders::_1;
  auto name = name_.append("#").append(
    std::to_string(reinterpret_cast<int32_t>(pipe)));
  auto conn = std::make_shared<Connection>(name, pipe, write_event);
  conn->SetCloseCallback(
    std::bind(&Server::Impl::RemoveConnection, this, _1));
  ConnectionAttorney::SetMessageCallback(conn, message_callback_);
  connection_map_.insert(std::make_pair(name, conn));
}

void Server::Impl::RemoveConnection(const ConnectionPtr& conn) {
  if (!DisconnectNamedPipe(ConnectionAttorney::Handle(conn))) {
    printf("DisconnectNamedPipe failed with %d.\n", GetLastError());
  }
  connection_map_.erase(conn->Name());
}


void Server::Impl::SendInAlertableThread() {
  std::for_each(std::begin(connection_map_),
                std::end(connection_map_),
                [](std::pair<std::string, ConnectionPtr> pair) {
    if (pair.second->State() == Connection::SEND_PENDDING) {
      ConnectionAttorney::AsyncWrite(pair.second);
    }
  });
}

// Server wrapper

Server::Server(const std::string& name)
  : impl_(new Impl(name)) {}

Server::~Server() {}

void Server::Listen() {
  impl_->Listen();
}

void Server::Stop() {
  impl_->Stop();
}

void Server::SetMessageCallback(const MessageCallback& cb) {
  impl_->SetMessageCallback(cb);
}

void Server::SetExceptionCallback(const ExceptionCallback& cb) {
  impl_->SetExceptionCallback(cb);
}

void Server::Broadcast(const std::string& message) {
  impl_->Broadcast(message);
}

void Server::CloseConnection(const std::string& name) {
  impl_->CloseConnection(name);
}

}  // namespace interprocess
