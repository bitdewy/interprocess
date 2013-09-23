//  Copyright 2013, bitdewy@gmail.com
//  Distributed under the Boost Software License, Version 1.0.
//  You may obtain a copy of the License at
//
//  http://www.boost.org/LICENSE_1_0.txt

#include <thread>
#include <algorithm>
#include <string>
#include "interprocess/client.h"
#include "interprocess/connection.h"

void OnMessage(
  const interprocess::ConnectionPtr& conn, const std::string& msg) {
  static int i = 0;
  printf("[%d]: %s\n", i++, msg.c_str());
}

void OnException(interprocess::Client* c, const std::exception_ptr& eptr) {
  try {
    if (eptr != std::exception_ptr()) {
      std::rethrow_exception(eptr);
    } else {
      c->Connection()->Send(c->Name());
      c->Connection()->Send("abcdefghijklmnopqrstuvwxyz");
    }
  } catch (const std::exception& e) {
    printf("Caught exception \"%s\"\n", e.what());
  }
}

void InThread() {
  auto name = std::to_string(std::this_thread::get_id().hash());
  auto client = interprocess::Client(name);
  client.SetMessageCallback(OnMessage);
  client.SetExceptionCallback(
    std::bind(OnException, &client, std::placeholders::_1));
  client.Connect("mynamedpipe");
  std::this_thread::sleep_for(std::chrono::seconds(10));
  client.Stop();
}

int main() {
  std::thread threads[1];
  for (auto i = 0; i < sizeof threads / sizeof threads[0]; ++i) {
    threads[i].swap(std::thread(InThread));
  }
  for (auto i = 0; i < sizeof threads / sizeof threads[0]; ++i) {
    threads[i].join();
  }
  return 0;
}
