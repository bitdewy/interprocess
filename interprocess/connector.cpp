//  Copyright 2013, bitdewy@gmail.com
//  Distributed under the Boost Software License, Version 1.0.
//  You may obtain a copy of the License at
//
//  http://www.boost.org/LICENSE_1_0.txt

#include "interprocess/connector.h"
#include <windows.h>
#include <cassert>
#include <string>

namespace interprocess {

Connector::Connector(const std::string& endpoint)
  : pipe_name_(std::string("\\\\.\\pipe\\").append(endpoint)),
    post_event_(CreateEvent(NULL, FALSE, FALSE, NULL)),
    close_event_(CreateEvent(NULL, FALSE, FALSE, NULL)) {
  assert(post_event_ != NULL && close_event_ != NULL);
}

Connector::~Connector() {
  Stop();
  CloseHandle(post_event_);
  CloseHandle(close_event_);
}

void Connector::Start() {
  connect_thread_.swap(
    std::thread(std::bind(&Connector::ConnectInThread, this)));
}

void Connector::Stop() {
  SetEvent(close_event_);
  if (connect_thread_.joinable()) {
    connect_thread_.join();
  }
}

void Connector::SetNewConnectionCallback(const NewConnectionCallback& cb) {
  new_connection_callback_ = cb;
}

void Connector::SetExceptionCallback(const ExceptionCallback& cb) {
  exception_callback_ = cb;
}

void Connector::MoveIOFunctionToAlertableThread(
  const std::function<void()>& cb) {
  async_io_callback_ = cb;
}

HANDLE Connector::CreateConnectionInstance() {
  HANDLE pipe = INVALID_HANDLE_VALUE;
  while (true) {
    pipe = CreateFile(
      pipe_name_.c_str(),            // pipe name
      GENERIC_READ | GENERIC_WRITE,  // read and write access
      0,                             // no sharing
      NULL,                          // default security attributes
      OPEN_EXISTING,                 // opens existing pipe
      FILE_FLAG_OVERLAPPED,          // default attributes
      NULL);                         // no template file

    // Break if the pipe handle is valid.
    if (pipe != INVALID_HANDLE_VALUE) {
      break;
    }

    // Exit if an error other than ERROR_PIPE_BUSY occurs.
    raise_exception_if([]() { return GetLastError() != ERROR_PIPE_BUSY; });

    // All pipe instances are busy, so wait for a while.
    raise_exception_if([this]() {
      return !WaitNamedPipe(pipe_name_.c_str(), kTimeout);
    });
  }
  return pipe;
}

void Connector::ConnectInThread() {
  std::exception_ptr eptr;
  try {
    auto pipe = CreateConnectionInstance();

    // The pipe connected; change to message-read mode.
    DWORD mode = PIPE_READMODE_MESSAGE;
    auto success = SetNamedPipeHandleState(
      pipe,     // pipe handle
      &mode,    // new pipe mode
      NULL,     // don't set maximum bytes
      NULL);    // don't set maximum time
    raise_exception_if([&]() { return !success; });

    HANDLE send_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    ScopeGuard send_event_guard([&] { CloseHandle(send_event); });
    raise_exception_if([&]() { return !send_event; }, send_event_guard);

    call_if_exist(new_connection_callback_, pipe, post_event_, send_event);

    HANDLE events[3] = { post_event_, send_event, close_event_ };

    while (true) {
      auto wait = WaitForMultipleObjectsEx(
        2,
        events,         // events object to wait for
        FALSE,          // wait any one
        INFINITE,       // wait indefinitely
        TRUE);          // alertable wait enabled

      switch (wait) {
      case WAIT_OBJECT_0:
        call_if_exist(async_io_callback_);
        break;

      case WAIT_OBJECT_0 + 1:
        call_if_exist(async_io_callback_);
        break;

      case WAIT_OBJECT_0 + 2:
        return;

      // The wait is satisfied by a completed read or write
      // operation. This allows the system to execute the
      // completion routine.
      case WAIT_IO_COMPLETION:
        break;

      default:
        raise_exception();
      }
    }
  } catch (...) {
    eptr = std::current_exception();
  }
  call_if_exist(exception_callback_, eptr);
}

}  // namespace interprocess
