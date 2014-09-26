#include <xzero/WallClock.h>
#include <xzero/DateTime.h>
#include <ctime>

namespace xzero {

class SystemClock : public WallClock {
 public:
  DateTime get() const override;
};

DateTime SystemClock::get() const {
  return DateTime(std::time(nullptr));
}

WallClock* WallClock::system() {
  static SystemClock clock;
  return &clock;
}

} // namespace xzero
