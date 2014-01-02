//  Copyright 2014, bitdewy@gmail.com
//  Distributed under the Boost Software License, Version 1.0.
//  You may obtain a copy of the License at
//
//  http://www.boost.org/LICENSE_1_0.txt

#include <cppunittest.h>
#include "interprocess/server.h"

namespace unittest {

BEGIN_TEST_MODULE_ATTRIBUTE()
  TEST_MODULE_ATTRIBUTE(L"Date", L"2013/10/18")
END_TEST_MODULE_ATTRIBUTE()

TEST_MODULE_INITIALIZE(ModuleInitialize) {
  using Microsoft::VisualStudio::CppUnitTestFramework::Logger;
  Logger::WriteMessage("In Module Initialize");
}

TEST_MODULE_CLEANUP(ModuleCleanup) {
  using Microsoft::VisualStudio::CppUnitTestFramework::Logger;
  Logger::WriteMessage("In Module Cleanup");
}

TEST_CLASS(ServerTest) {
 public:
  ServerTest() {
    using Microsoft::VisualStudio::CppUnitTestFramework::Logger;
    Logger::WriteMessage("In TestClass");
  }

  ~ServerTest() {
    using Microsoft::VisualStudio::CppUnitTestFramework::Logger;
    Logger::WriteMessage("In ~TestClass");
  }

  TEST_CLASS_INITIALIZE(ClassInitialize) {
    using Microsoft::VisualStudio::CppUnitTestFramework::Logger;
    Logger::WriteMessage("In Class Initialize");
  }

  TEST_CLASS_CLEANUP(ClassCleanup) {
    using Microsoft::VisualStudio::CppUnitTestFramework::Logger;
    Logger::WriteMessage("In Class Cleanup");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(TestServerStart)
    TEST_OWNER(L"Bitdewy")
    TEST_PRIORITY(1)
  END_TEST_METHOD_ATTRIBUTE()

  TEST_METHOD(TestServerStart) {
    using Microsoft::VisualStudio::CppUnitTestFramework::Logger;
    using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
    interprocess::Server server("server");
    Logger::WriteMessage("In TestMethod");
    Assert::AreEqual(0, 0);
  }
};

}  // namespace unittest
