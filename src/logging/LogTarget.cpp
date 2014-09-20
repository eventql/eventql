#include <xzero/logging/LogTarget.h>
#include <stdio.h>
#include <stdarg.h>

namespace xzero {

// {{{ ConsoleLogger
class ConsoleLogger : public LogTarget {
 public:
  void trace(const std::string& msg) override {
    fprintf(stderr, "[trace] %s\n", msg.c_str());
  }

  void debug(const std::string& msg) override {
    fprintf(stderr, "[debug] %s\n", msg.c_str());
  }

  void info(const std::string& msg) override {
    fprintf(stderr, "[info] %s\n", msg.c_str());
  }

  void warn(const std::string& msg) override {
    fprintf(stderr, "[warning] %s\n", msg.c_str());
  }

  void error(const std::string& msg) override {
    fprintf(stderr, "[error] %s\n", msg.c_str());
  }

  static ConsoleLogger* get();
};

ConsoleLogger* ConsoleLogger::get() {
  static ConsoleLogger instance;
  return &instance;
}
// }}}
// {{{ LogTarget
LogTarget* LogTarget::console() {
  return ConsoleLogger::get();
}

LogTarget* LogTarget::syslog() {
  return nullptr; // TODO
}
// }}}

} // namespace xzero
