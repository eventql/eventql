#include <xzero/http/HttpDateGenerator.h>
#include <xzero/WallClock.h>
#include <xzero/DateTime.h>
#include <cassert>
#include <ctime>

namespace xzero {

HttpDateGenerator::HttpDateGenerator(WallClock* clock)
  : clock_(clock),
    current_(0),
    buffer_() {
}

void HttpDateGenerator::update() {
  assert(clock_ != nullptr);

  DateTime now = clock_->get();
  if (current_ != now) {
    std::time_t ts = now.unixtime();
    if (struct tm* tm = std::gmtime(&ts)) {
      char buf[256];

      ssize_t n = std::strftime(buf, sizeof(buf), "%a, %d %b %Y %T GMT", tm);
      if (n != 0) {
        current_ = now;
        buffer_.clear();
        buffer_.push_back(buf, n);
      }
    }
  }
}

void HttpDateGenerator::fill(Buffer* target) {
  assert(target != nullptr);

  update();
  target->push_back(buffer_);
}

} // namespace xzero
