namespace stx {

inline constexpr TimeSpan::TimeSpan(UnixTime start, Duration duration)
    : start_(start), duration_(duration) {
}

inline constexpr TimeSpan::TimeSpan(UnixTime start, UnixTime end)
    : start_(start), duration_(end - start) {
}

inline constexpr TimeSpan::TimeSpan(const TimeSpan& other) 
    : start_(other.start_), duration_(other.duration_) {
}

inline constexpr TimeSpan::TimeSpan()
    : start_(0), duration_(Duration::Zero) {
}

inline constexpr UnixTime TimeSpan::start() const {
  return start_;
}

inline constexpr Duration TimeSpan::duration() const {
  return duration_;
}

inline constexpr UnixTime TimeSpan::end() const {
  return start_ + duration_;
}

inline constexpr TimeSpan TimeSpan::backward(Duration by) const {
  return TimeSpan(start_ - by, duration_);
}

inline constexpr TimeSpan TimeSpan::forward(Duration by) const {
  return TimeSpan(start_ + by, duration_);
}

inline constexpr bool TimeSpan::operator==(const TimeSpan& other) const {
  return start_ == other.start_ && duration_ == other.duration_;
}

inline constexpr bool TimeSpan::operator!=(const TimeSpan& other) const {
  return start_ != other.start_ || duration_ != other.duration_;
}

} // namespace stx
