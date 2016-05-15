#include <eventql/util/MonotonicTime.h>
#include <eventql/util/stringutil.h>

namespace util {

std::string inspect(const MonotonicTime& value) {
  return StringUtil::format("$0", value.milliseconds());
}

template<>
std::string StringUtil::toString<MonotonicTime>(MonotonicTime value) {
  return inspect(value);
}

template<>
std::string StringUtil::toString<const MonotonicTime&>(const MonotonicTime& value) {
  return inspect(value);
}

} // namespace util
