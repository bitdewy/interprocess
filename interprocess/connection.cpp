//  Copyright 2014, bitdewy@gmail.com
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
    SetEvent(self->cancel_io_event_.get());
    return;
  }
  bool io = false;
  if ((err == 0) && (readed != 0)) {
    auto message = std::string(self->read_buf_, readed);
    io = self->AsyncRead(CompletedReadRoutine);
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
    SetEvent(self->cancel_io_event_.get());
    assert(("write operation should not be cancelled", false));
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
    io = pendding ?
      self->AsyncWrite(message, CompletedWriteRoutine) :
      self->AsyncRead(CompletedReadRoutine);
  }

  if (!io) {
    self->Shutdown();
  }
}

VOID WINAPI CompletedReadRoutineForWait(
  DWORD err, DWORD readed, LPOVERLAPPED overlap) {
  auto context = (Connection::IoCompletionRoutine*)overlap;
  auto self = context->self;

  if (err == ERROR_OPERATION_ABORTED) {
    SetEvent(self->cancel_io_event_.get());
    return;
  }
  bool io = false;
  if ((err == 0) && (readed != 0)) {
    auto message = std::string(self->read_buf_, readed);
    std::unique_lock<std::mutex> lock(self->transact_message_buffer_mutex_);
    message.swap(self->transact_message_buffer_);
    self->transact_message_buffer_cond.notify_all();
    io = self->AsyncRead(CompletedReadRoutine);
  }

  if (!io) {
    self->Shutdown();
  }
}

VOID WINAPI CompletedWriteRoutineForWait(
  DWORD err, DWORD written, LPOVERLAPPED overlap) {
  auto context = (Connection::IoCompletionRoutine*)overlap;
  auto self = context->self;
  if (err == ERROR_OPERATION_ABORTED) {
    SetEvent(self->cancel_io_event_.get());
    assert(("write operation should not be cancelled", false));
    return;
  }
  bool io = false;
  // The write operation has finished, so read() the next request (if
  // there is no error) or continue write if necessary.
  if ((err == 0) && (written == self->write_size_)) {
    io = self->AsyncRead(CompletedReadRoutineForWait);
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
  AsyncRead(CompletedReadRoutine);
}

Connection::~Connection() {
  CancelIo(pipe_.get());
}

std::string Connection::Name() const {
  return name_;
}

void Connection::Send(const std::string& message) {
  {
    std::unique_lock<std::mutex> lock(sending_queue_mutex_);
    assert(("message buffer overflow", message.size() < kBufferSize));
    sending_queue_.push_back(message);
    state_ = SEND_PENDDING;
  }
  SetEvent(post_event_.get());
}

std::string Connection::TransactMessage(std::string message) {
  assert(io_thread_id_ != std::this_thread::get_id() && message.size());
  std::unique_lock<std::mutex> lock(transact_message_buffer_mutex_);
  message.swap(transact_message_buffer_);
  SetEvent(send_event_.get());
  transact_message_buffer_cond.wait(lock, [this]() {
    return transact_message_buffer_.empty();
  });

  transact_message_buffer_cond.wait_for(
    lock,
    std::chrono::seconds(2),
    [this]() { return !transact_message_buffer_.empty(); });
  std::string result;
  result.swap(transact_message_buffer_);
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
  return pipe_.get();
}

bool Connection::AsyncRead(LPOVERLAPPED_COMPLETION_ROUTINE cb) {
  if (disconnecting_ && sending_queue_.empty()) {
    return false;
  }
  ZeroMemory(read_buf_, sizeof read_buf_);
  auto read = ReadFileEx(
    pipe_.get(),
    read_buf_,
    kBufferSize * sizeof read_buf_[0],
    (LPOVERLAPPED)&io_overlap_,
    cb);
  return !!read;
}

bool Connection::AsyncWrite() {
  // must cancel read operation first
  if (CancelIo(pipe_.get())) {
    while (WAIT_OBJECT_0 != WaitForSingleObjectEx(
      cancel_io_event_.get(), INFINITE, TRUE)) {
      continue;
    }
  }
  std::string message;
  {
    std::unique_lock<std::mutex> lock(sending_queue_mutex_);
    assert(("no more message to send", !sending_queue_.empty()));
    message = sending_queue_.front();
    sending_queue_.pop_front();
  }
  return AsyncWrite(message, CompletedWriteRoutine);
}

bool Connection::AsyncWaitWrite() {
  if (CancelIo(pipe_.get())) {
    while (WAIT_OBJECT_0 != WaitForSingleObjectEx(
      cancel_io_event_.get(), INFINITE, TRUE)) {
      continue;
    }
  }
  std::string message;
  std::unique_lock<std::mutex> lock(transact_message_buffer_mutex_);
  message.swap(transact_message_buffer_);
  transact_message_buffer_cond.notify_all();
  return AsyncWrite(message, CompletedWriteRoutineForWait);
}

bool Connection::AsyncWrite(
  const std::string& message, LPOVERLAPPED_COMPLETION_ROUTINE cb) {
  write_size_ = message.size();
#pragma warning(disable:4996)
  // already checked message length when push it into sendding queue
  message.copy(std::begin(write_buf_), write_size_);
#pragma warning(default:4996)

  auto write = WriteFileEx(
    pipe_.get(),
    write_buf_,
    write_size_,
    (LPOVERLAPPED)&io_overlap_,
    cb);
  return !!write;
}

}  // namespace interprocess
