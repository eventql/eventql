#include <cortex-base/logging/LogAggregator.h>
#include <cortex-base/logging.h>

int main() {
  cortex::LogAggregator::get().setLogLevel(cortex::LogLevel::trace);
  cortex::logDebug("example.logging-example", "Hello, %s", "World");
}
