// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/logging/LogAggregator.h>
#include <cortex-base/logging/LogSource.h>
#include <cortex-base/logging/LogTarget.h>
#include <cortex-base/logging.h>
#include <cortex-base/Buffer.h>
#include <stdlib.h>

namespace cortex {

LogAggregator::LogAggregator()
    : LogAggregator(LogLevel::Error, LogTarget::console()) {
}

LogAggregator::LogAggregator(LogLevel logLevel, LogTarget* logTarget)
    : target_(logTarget),
      logLevel_(logLevel),
      enabledSources_(),
      activeSources_() {
}

LogAggregator::~LogAggregator() {
  for (auto& activeSource: activeSources_)
    delete activeSource.second;

  activeSources_.clear();
}

void LogAggregator::setLogTarget(LogTarget* target) {
  target_ = target;
}

void LogAggregator::registerSource(LogSource* source) {
  std::lock_guard<std::mutex> _guard(mutex_);
  activeSources_[source->componentName()] = source;
}

void LogAggregator::unregisterSource(LogSource* source) {
  std::lock_guard<std::mutex> _guard(mutex_);
  auto i = activeSources_.find(source->componentName());
  activeSources_.erase(i);
}

LogSource* LogAggregator::getSource(const std::string& componentName) {
  std::lock_guard<std::mutex> _guard(mutex_);

  auto i = activeSources_.find(componentName);

  if (i != activeSources_.end())
    return i->second;
  else {
    LogSource* source = new LogSource(componentName);
    activeSources_[componentName] = source;
    return source;
  }
}

LogAggregator& LogAggregator::get() {
  static LogAggregator aggregator;
  return aggregator;
}

CORTEX_INIT static void initializeLogAggregator() {
  try {
    if (const char* target = getenv("CORTEX_LOGTARGET")) {
      if (iequals(target, "console")) {
        LogAggregator::get().setLogTarget(LogTarget::console());
      } else if (iequals(target, "syslog")) {
        LogAggregator::get().setLogTarget(LogTarget::syslog());
      } else {
        logError("logging", "Unknown log target \"%s\"", target);
      }
    }

    if (const char* level = getenv("CORTEX_LOGLEVEL")) {
      LogAggregator::get().setLogLevel(to_loglevel(level));
    }
  } catch (...) {
    // eat my lunch pack
  }
}

} // namespace cortex
