//  Copyright 2013, bitdewy@gmail.com
//  Distributed under the Boost Software License, Version 1.0.
//  You may obtain a copy of the License at
//
//  http://www.boost.org/LICENSE_1_0.txt

#include "interprocess/connection.h"
#include <cassert>
#include <string>

namespace interprocess {

VOID WINAPI CompletedReadRoutine(
  DWORD err, DWORD readed, LPOVERLAPPED overlap) {
  auto context = (Connection::IoCompletionRoutine*)overlap;
  auto self = context->self;

  if (err == ERROR_OPERATION_ABORTED) {
    SetEvent(self->cancel_io_event_);
    return;
  }
  bool io = false;
  if ((err == 0) && (readed != 0)) {
    auto message = std::string(self->read_buf_, readed);
    io = self->AsyncRead();
    if (!message.empty()) {
      self->message_callback_(self->shared_from_this(), message);
    }
  }

  if (!io) {
    self->Shutdown();
  }
}

VOID WINAPI CompletedWriteRoutine(
  DWORD err, DWORD written, LPOVERLAPPED overlap) {
  auto context = (Connection::IoCompletionRoutine*)overlap;
  auto self = context->self;
  if (err == ERROR_OPERATION_ABORTED) {
    SetEvent(self->cancel_io_event_);
    assert(false);
    return;
  }
  bool io = false;
  // The write operation has finished, so read() the next request (if
  // there is no error) or continue write if necessary.
  if ((err == 0) && (written == self->write_size_)) {
    bool pendding = false;
    std::string message;
    {
      std::unique_lock<std::mutex> lock(self->sending_queue_mutex_);
      pendding = !self->sending_queue_.empty();
      if (pendding) {
        message = self->sending_queue_.front();
        self->sending_queue_.pop_front();
        self->state_ = Connection::SEND_PENDDING;
      } else {
        self->state_ = Connection::CONNECTED;
      }
    }
    io = pendding ? self->AsyncWrite(message) : self->AsyncRead();
  }

  if (!io) {
    self->Shutdown();
  }
}

Connection::Connection(
  const std::string& name, HANDLE pipe, HANDLE post_event, HANDLE send_event)
  : name_(name),
    state_(UNKNOW),
    pipe_(pipe),
    post_event_(post_event),
    send_event_(send_event),
    cancel_io_event_(CreateEvent(NULL, FALSE, FALSE, NULL)),
    write_size_(0),
    io_thread_id_(std::this_thread::get_id()),
    disconnecting_(false) {
  ZeroMemory(read_buf_, sizeof read_buf_);
  ZeroMemory(write_buf_, sizeof write_buf_);
  ZeroMemory(&io_overlap_, sizeof io_overlap_);
  io_overlap_.self = this;
  AsyncRead();
}

Connection::~Connection() {
  CancelIo(pipe_);
  CloseHandle(pipe_);
  CloseHandle(cancel_io_event_);
}

std::string Connection::Name() const {
  return name_;
}

void Connection::Post(const std::string& message) {
  {
    std::unique_lock<std::mutex> lock(sending_queue_mutex_);
    assert(message.size() < kBufferSize);
    sending_queue_.push_back(message);
    state_ = SEND_PENDDING;
  }
  SetEvent(post_event_);
}

std::string Connection::Send(const std::string& message) {
  assert(io_thread_id_ != std::this_thread::get_id());
  std::unique_lock<std::mutex> lock(sync_response_buffer_mutex_);
  sync_message_buffer_cond.wait_for(
    lock,
    std::chrono::seconds(2),
    [this]() { return !sync_response_buffer_.empty(); });
  std::string result;
  result.swap(sync_response_buffer_);
  return result;
}

void Connection::Close() {
  if (std::this_thread::get_id() != io_thread_id_) {
    Shutdown();
  } else {
    disconnecting_ = true;
  }
}

void Connection::SetCloseCallback(const CloseCallback& cb) {
  close_callback_ = cb;
}

Connection::StateE Connection::State() const {
  return state_;
}

void Connection::Shutdown() {
  close_callback_(shared_from_this());
}

void Connection::SetMessageCallback(const MessageCallback& cb) {
  message_callback_ = cb;
}

HANDLE Connection::Handle() const {
  return pipe_;
}

bool Connection::AsyncRead() {
  if (disconnecting_ && sending_queue_.empty()) {
    return false;
  }
  ZeroMemory(read_buf_, sizeof read_buf_);
  auto read = ReadFileEx(
    pipe_,
    read_buf_,
    kBufferSize * sizeof read_buf_[0],
    (LPOVERLAPPED)&io_overlap_,
    (LPOVERLAPPED_COMPLETION_ROUTINE)CompletedReadRoutine);
  return !!read;
}

bool Connection::AsyncWrite() {
  // must cancel read operation first
  if (CancelIo(pipe_)) {
    auto h = cancel_io_event_;
    while (WAIT_OBJECT_0 != WaitForSingleObjectEx(h, INFINITE, TRUE)) {
      continue;
    }
  }
  std::string message;
  {
    std::unique_lock<std::mutex> lock(sending_queue_mutex_);
    assert(!sending_queue_.empty());
    message = sending_queue_.front();
    sending_queue_.pop_front();
  }
  return AsyncWrite(message);
}

bool Connection::AsyncWrite(const std::string& message) {
  write_size_ = message.size();
#pragma warning(disable:4996)
  // already checked message length when push it into sendding queue
  message.copy(std::begin(write_buf_), write_size_);
#pragma warning(default:4996)

  auto write = WriteFileEx(
    pipe_,
    write_buf_,
    write_size_,
    (LPOVERLAPPED)&io_overlap_,
    (LPOVERLAPPED_COMPLETION_ROUTINE)CompletedWriteRoutine);
  return !!write;
}

}  // namespace interprocess
