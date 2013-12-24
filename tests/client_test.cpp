//  Copyright 2013, bitdewy@gmail.com
//  Distributed under the Boost Software License, Version 1.0.
//  You may obtain a copy of the License at
//
//  http://www.boost.org/LICENSE_1_0.txt

#include <ppltasks.h>
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
    if (eptr) {
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
  if (client.Connect("mynamedpipe", 1000)) {
    auto response = client.Connection()->TransactMessage("async & wait");
    printf("TransactMessage response: %s\n", response.c_str());
    std::this_thread::sleep_for(std::chrono::seconds(1));
    client.Connection()->Send(client.Name());
    client.Connection()->Send("abcdefghijklmnopqrstuvwxyz");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    client.Stop();
  }
}

int main() {
  Concurrency::task_group tasks;
  int count = 10;
  while (count--) {
    tasks.run(std::function<void()>(InThread));
  }
  tasks.wait();
  return 0;
}
