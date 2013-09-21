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
    write_event_(CreateEvent(NULL, FALSE, FALSE, NULL)),
    close_event_(CreateEvent(NULL, FALSE, FALSE, NULL)) {
  assert(write_event_ != NULL && close_event_ != NULL);
}

Connector::~Connector() {
  Stop();
  CloseHandle(write_event_);
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

void Connector::MoveIOFunctionToAlertableThread(
  const std::function<void()>& cb) {
  async_io_callback_ = cb;
}

void Connector::ConnectInThread() {
  HANDLE pipe = INVALID_HANDLE_VALUE;
  while (1) {
    pipe = CreateFile(
      pipe_name_.c_str(),    // pipe name
      GENERIC_READ |         // read and write access
      GENERIC_WRITE,
      0,                     // no sharing
      NULL,                  // default security attributes
      OPEN_EXISTING,         // opens existing pipe
      FILE_FLAG_OVERLAPPED,  // default attributes
      NULL);                 // no template file

    // Break if the pipe handle is valid.
    if (pipe != INVALID_HANDLE_VALUE) {
      break;
    }

    // Exit if an error other than ERROR_PIPE_BUSY occurs.
    if (GetLastError() != ERROR_PIPE_BUSY) {
      printf(TEXT("Could not open pipe. GLE=%d\n"), GetLastError());
      assert(false);
    }

    // All pipe instances are busy, so wait for 20 seconds.
    if (!WaitNamedPipe(pipe_name_.c_str(), kTimeout)) {
      printf("Could not open pipe: 20 second wait timed out.");
      assert(false);
    }
  }

  // The pipe connected; change to message-read mode.
  DWORD mode = PIPE_READMODE_MESSAGE;
  auto success = SetNamedPipeHandleState(
    pipe,    // pipe handle
    &mode,  // new pipe mode
    NULL,     // don't set maximum bytes
    NULL);    // don't set maximum time
  if (!success) {
    printf(TEXT("SetNamedPipeHandleState failed. GLE=%d\n"), GetLastError());
    assert(false);
  }
  if (new_connection_callback_) {
    new_connection_callback_(pipe, write_event_);
  }

  HANDLE events[2] = { write_event_, close_event_ };

  for (;;) {
    auto wait = WaitForMultipleObjectsEx(
      2,
      events,         // events object to wait for
      FALSE,
      INFINITE,       // wait indefinitely
      TRUE);          // alertable wait enabled

    switch (wait) {
    case WAIT_OBJECT_0:
      if (async_io_callback_) {
        async_io_callback_();
      }
      break;

    case WAIT_OBJECT_0 + 1:
      return;

    // The wait is satisfied by a completed read or write
    // operation. This allows the system to execute the
    // completion routine.
    case WAIT_IO_COMPLETION:
      break;

    // An error occurred in the wait function.
    default:
      printf("ConnectionPtr::Wait() (%d)\n", GetLastError());
      return;
    }
  }
}

}  // namespace interprocess
