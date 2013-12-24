//  Copyright 2013, bitdewy@gmail.com
//  Distributed under the Boost Software License, Version 1.0.
//  You may obtain a copy of the License at
//
//  http://www.boost.org/LICENSE_1_0.txt

#ifndef INTERPROCESS_CONNECTION_H_
#define INTERPROCESS_CONNECTION_H_

#include <windows.h>
#include <condition_variable>
#include <thread>
#include <mutex>
#include <memory>
#include <string>
#include <deque>
#include "interprocess/types.h"

namespace interprocess {

class Connection : public std::enable_shared_from_this<Connection> {
 public:
  enum StateE {
    UNKNOW,
    SEND_PENDDING,
    CONNECTED,
  };
  Connection(
    const std::string& name, HANDLE pipe, HANDLE post_event, HANDLE send_event);
  Connection(const Connection&) = delete;
  Connection& operator=(const Connection&) = delete;
  ~Connection();
  std::string Name() const;
  void Send(const std::string& message);
  std::string TransactMessage(std::string message);
  void Close();
  void SetCloseCallback(const CloseCallback& cb);
  Connection::StateE State() const;

 private:
  void Shutdown();
  void SetMessageCallback(const MessageCallback& cb);
  HANDLE Handle() const;
  bool AsyncRead(LPOVERLAPPED_COMPLETION_ROUTINE cb);
  bool AsyncWrite();
  bool AsyncWaitWrite();
  bool AsyncWrite(
    const std::string& message, LPOVERLAPPED_COMPLETION_ROUTINE cb);
  typedef std::deque<std::string> SendingQueue;
  struct IoCompletionRoutine {
    OVERLAPPED overlap;
    Connection* self;
  };

  CloseCallback close_callback_;
  MessageCallback message_callback_;
  std::string name_;
  StateE state_;
  HANDLE pipe_;
  HANDLE post_event_;
  HANDLE send_event_;
  HANDLE cancel_io_event_;
  DWORD write_size_;
  char read_buf_[kBufferSize];
  char write_buf_[kBufferSize];
  std::mutex sending_queue_mutex_;
  SendingQueue sending_queue_;
  std::condition_variable transact_message_buffer_cond;
  std::string transact_message_buffer_;
  std::mutex transact_message_buffer_mutex_;
  IoCompletionRoutine io_overlap_;
  std::thread::id io_thread_id_;
  bool disconnecting_;

  friend class ConnectionAttorney;

  friend VOID WINAPI CompletedReadRoutine(DWORD, DWORD, LPOVERLAPPED);
  friend VOID WINAPI CompletedWriteRoutine(DWORD, DWORD, LPOVERLAPPED);
  friend VOID WINAPI CompletedReadRoutineForWait(DWORD, DWORD, LPOVERLAPPED);
  friend VOID WINAPI CompletedWriteRoutineForWait(DWORD, DWORD, LPOVERLAPPED);
};

class ConnectionAttorney {
  friend class Server;
  friend class Client;

 private:
  static void SetMessageCallback(ConnectionPtr& c, const MessageCallback& cb) {
    c->SetMessageCallback(cb);
  }

  static HANDLE Handle(const ConnectionPtr& c) {
    return c->Handle();
  }

  static void AsyncWrite(const ConnectionPtr& c) {
    c->AsyncWrite();
  }

  static void AsyncWaitWrite(const ConnectionPtr& c) {
    c->AsyncWaitWrite();
  }
};

}  // namespace interprocess

#endif  // INTERPROCESS_CONNECTION_H_
