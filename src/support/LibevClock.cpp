#include <xzero/support/libev/LibevClock.h>

namespace xzero {
namespace support {

LibevClock::LibevClock(ev::loop_ref loop) :
  loop_(loop) {
}

DateTime LibevClock::get() const {
  return DateTime(loop_.now());
}

} // namespace support
} // namespace xzero
