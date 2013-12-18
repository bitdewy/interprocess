//  Copyright 2013, bitdewy@gmail.com
//  Distributed under the Boost Software License, Version 1.0.
//  You may obtain a copy of the License at
//
//  http://www.boost.org/LICENSE_1_0.txt

#include <thread>
#include <string>
#include "interprocess/server.h"
#include "interprocess/connection.h"

void OnMessage(
  const interprocess::ConnectionPtr& conn, const std::string& msg) {
  static int i = 0;
  printf("[%d]: %s\n", i++, msg.c_str());
  conn->Send(msg);
}

int main() {
  interprocess::Server server("mynamedpipe");
  server.SetMessageCallback(OnMessage);
  server.Listen();
  std::this_thread::sleep_for(std::chrono::hours(30));
  server.Broadcast("0");
  server.Stop();
  return 0;
}
