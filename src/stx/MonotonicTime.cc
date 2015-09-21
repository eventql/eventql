#include <stx/MonotonicTime.h>
#include <stx/StringUtil.h>

namespace stx {

std::string inspect(const MonotonicTime& value) {
  return StringUtil::format("$1ms", value.microseconds());
}

} // namespace stx
