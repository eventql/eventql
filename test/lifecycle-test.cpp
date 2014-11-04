// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/logging/LogSource.h>

class Test {
 public:
  explicit Test(const char* id);
  ~Test();
  void f();

 private:
  std::string id_;
  static xzero::LogSource logger_;
};

xzero::LogSource Test::logger_("Test");

Test::Test(const char* id) : id_(id) {
  logger_.debug("Test[%s]", id_.c_str());
}

Test::~Test() {
  logger_.debug("~Test[%s]", id_.c_str());
}

void Test::f() {
  logger_.info("Test.f[%s]: %p", id_.c_str(), this);
}

int main() {
  /* xzero::LogAggregator::get().configure("+Test:-Foo"); */

  Test a("a");
  a.f();
  Test b("b");
  b.f();

  return 0;
}
