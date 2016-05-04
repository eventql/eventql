namespace stx {

inline constexpr TimeRange::TimeRange(UnixTime start, Duration duration)
    : start_(start), duration_(duration) {
}

inline constexpr TimeRange::TimeRange(UnixTime start, UnixTime end)
    : start_(start), duration_(end - start) {
}

inline constexpr TimeRange::TimeRange(const TimeRange& other) 
    : start_(other.start_), duration_(other.duration_) {
}

inline constexpr TimeRange::TimeRange()
    : start_(0), duration_(Duration::Zero) {
}

inline constexpr UnixTime TimeRange::start() const {
  return start_;
}

inline constexpr Duration TimeRange::duration() const {
  return duration_;
}

inline constexpr UnixTime TimeRange::end() const {
  return start_ + duration_;
}

inline constexpr TimeRange TimeRange::backward(Duration by) const {
  return TimeRange(start_ - by, duration_);
}

inline constexpr TimeRange TimeRange::forward(Duration by) const {
  return TimeRange(start_ + by, duration_);
}

inline constexpr bool TimeRange::operator==(const TimeRange& other) const {
  return start_ == other.start_ && duration_ == other.duration_;
}

inline constexpr bool TimeRange::operator!=(const TimeRange& other) const {
  return start_ != other.start_ || duration_ != other.duration_;
}

} // namespace stx
