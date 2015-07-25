// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <cortex-base/Api.h>
#include <cortex-base/sysconfig.h>
#include <cortex-base/logging/LogLevel.h>
#include <string>
#include <mutex>
#include <unordered_map>

namespace cortex {

class LogSource;
class LogTarget;

/**
 * Logging Aggregator Service.
 */
class CORTEX_API LogAggregator {
 public:
  LogAggregator();
  LogAggregator(LogLevel logLevel, LogTarget* logTarget);
  ~LogAggregator();

  LogLevel logLevel() const CORTEX_NOEXCEPT { return logLevel_; }
  void setLogLevel(LogLevel level) { logLevel_ = level; }

  LogTarget* logTarget() const CORTEX_NOEXCEPT { return target_; }
  void setLogTarget(LogTarget* target);

  void registerSource(LogSource* source);
  void unregisterSource(LogSource* source);
  LogSource* getSource(const std::string& className);

  static LogAggregator& get();

 private:
  LogTarget* target_;
  LogLevel logLevel_;
  std::mutex mutex_;
  std::unordered_map<std::string, bool> enabledSources_;
  std::unordered_map<std::string, LogSource*> activeSources_;
};

}  // namespace cortex
