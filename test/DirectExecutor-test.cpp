#include <gtest/gtest.h>
#include <xzero/executor/DirectExecutor.h>

using namespace xzero;

TEST(DirectExecutor, executeMe) {
  DirectExecutor executor;
  int runCounter = 0;

  executor.execute([&]() {
    runCounter++;
  });

  ASSERT_EQ(1, runCounter);
}

TEST(DirectExecutor, exceptionHandling) {
  DirectExecutor executor;
  int handlerTriggered = 0;

  executor.setExceptionHandler([&](const std::exception& e) {
    handlerTriggered++;
  });

  executor.execute([]() {
    throw std::runtime_error("some error");
  });

  ASSERT_EQ(1, handlerTriggered);
}
