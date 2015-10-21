
namespace stx {

inline constexpr MonotonicTime::MonotonicTime()
    : nanosecs_(0) {
}

inline constexpr MonotonicTime::MonotonicTime(const MonotonicTime& other)
    : nanosecs_(other.nanosecs_) {
}

inline constexpr MonotonicTime::MonotonicTime(uint64_t nanosecs)
    : nanosecs_(nanosecs) {
}

inline constexpr uint64_t MonotonicTime::seconds() const {
  return nanosecs_ / 1000000000;
}

inline constexpr uint64_t MonotonicTime::milliseconds() const {
  return nanosecs_ / 1000000;
}

inline constexpr uint64_t MonotonicTime::microseconds() const {
  return nanosecs_ / 1000;
}

inline constexpr uint64_t MonotonicTime::nanoseconds() const {
  return nanosecs_;
}

inline constexpr Duration MonotonicTime::operator-(const MonotonicTime& other) const {
  return Duration(microseconds() - other.microseconds());
}

inline constexpr MonotonicTime MonotonicTime::operator+(const Duration& other) const {
  return MonotonicTime(nanosecs_ + other.microseconds() * 1000);
}

inline constexpr bool MonotonicTime::operator==(const MonotonicTime& other) const {
  return nanosecs_ == other.nanosecs_;
}

inline constexpr bool MonotonicTime::operator!=(const MonotonicTime& other) const {
  return nanosecs_ != other.nanosecs_;
}

inline constexpr bool MonotonicTime::operator<=(const MonotonicTime& other) const {
  return nanosecs_ <= other.nanosecs_;
}

inline constexpr bool MonotonicTime::operator>=(const MonotonicTime& other) const {
  return nanosecs_ >= other.nanosecs_;
}

inline constexpr bool MonotonicTime::operator<(const MonotonicTime& other) const {
  return nanosecs_ < other.nanosecs_;
}

inline constexpr bool MonotonicTime::operator>(const MonotonicTime& other) const {
  return nanosecs_ > other.nanosecs_;
}

inline constexpr bool MonotonicTime::operator!() const {
  return nanosecs_ == 0;
}

} // namespace stx
