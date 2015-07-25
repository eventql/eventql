// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/logging/LogTarget.h>
#include <unordered_map>
#include <mutex>
#include <stdio.h>
#include <stdarg.h>
#include <sys/syslog.h>

namespace cortex {

// {{{ ConsoleLogger
class ConsoleLogger : public LogTarget {
 public:
  void trace(const std::string& msg) override {
    std::lock_guard<std::mutex> _lg(lock_);
    fprintf(stderr, "[thread:%d] [trace] %s\n", threadId(), msg.c_str());
  }

  void debug(const std::string& msg) override {
    std::lock_guard<std::mutex> _lg(lock_);
    fprintf(stderr, "[thread:%d] [debug] %s\n", threadId(), msg.c_str());
  }

  void info(const std::string& msg) override {
    std::lock_guard<std::mutex> _lg(lock_);
    fprintf(stderr, "[thread:%d] [info] %s\n", threadId(), msg.c_str());
  }

  void notice(const std::string& msg) override {
    std::lock_guard<std::mutex> _lg(lock_);
    fprintf(stderr, "[thread:%d] [notice] %s\n", threadId(), msg.c_str());
  }

  void warn(const std::string& msg) override {
    std::lock_guard<std::mutex> _lg(lock_);
    fprintf(stderr, "[thread:%d] [warning] %s\n", threadId(), msg.c_str());
  }

  void error(const std::string& msg) override {
    std::lock_guard<std::mutex> _lg(lock_);
    fprintf(stderr, "[thread:%d] [error] %s\n", threadId(), msg.c_str());
  }

  int threadId() {
    pthread_t tid = pthread_self();
    auto i = threadMap_.find(tid);
    if (i != threadMap_.end()) {
      return i->second;
    }
    threadMap_[tid] = threadMap_.size() + 1;
    return threadMap_.size();
  }

  std::mutex lock_;
  std::unordered_map<pthread_t, int> threadMap_;

  static ConsoleLogger* get();
};

ConsoleLogger* ConsoleLogger::get() {
  static ConsoleLogger instance;
  return &instance;
}
// }}}
// {{{ SyslogLogger
class SyslogLogger : public LogTarget {
 public:
  void trace(const std::string& msg) override {
    ::syslog(LOG_DEBUG, "[thread:%d] %s\n", threadId(), msg.c_str());
  }

  void debug(const std::string& msg) override {
    ::syslog(LOG_DEBUG, "[thread:%d] %s\n", threadId(), msg.c_str());
  }

  void info(const std::string& msg) override {
    ::syslog(LOG_INFO, "[thread:%d] %s\n", threadId(), msg.c_str());
  }

  void notice(const std::string& msg) override {
    ::syslog(LOG_NOTICE, "[thread:%d] %s\n", threadId(), msg.c_str());
  }

  void warn(const std::string& msg) override {
    ::syslog(LOG_WARNING, "[thread:%d] %s\n", threadId(), msg.c_str());
  }

  void error(const std::string& msg) override {
    ::syslog(LOG_ERR, "[thread:%d] %s\n", threadId(), msg.c_str());
  }

  int threadId() {
    std::lock_guard<std::mutex> _lg(lock_);
    pthread_t tid = pthread_self();
    auto i = threadMap_.find(tid);
    if (i != threadMap_.end()) {
      return i->second;
    }
    threadMap_[tid] = threadMap_.size() + 1;
    return threadMap_.size();
  }

  std::mutex lock_;
  std::unordered_map<pthread_t, int> threadMap_;

  static SyslogLogger* get();
};

SyslogLogger* SyslogLogger::get() {
  static SyslogLogger instance;
  return &instance;
}
// }}}
// {{{ LogTarget
LogTarget* LogTarget::console() {
  return ConsoleLogger::get();
}

LogTarget* LogTarget::syslog() {
  return SyslogLogger::get();
}
// }}}

} // namespace cortex
