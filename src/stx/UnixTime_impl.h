namespace stx {

inline constexpr UnixTime::UnixTime(uint64_t utc_time) :
    utc_micros_(utc_time) {
}

inline constexpr bool UnixTime::operator==(const UnixTime& other) const {
  return utc_micros_ == other.utc_micros_;
}

inline constexpr bool UnixTime::operator<(const UnixTime& other) const {
  return utc_micros_ < other.utc_micros_;
}

inline constexpr bool UnixTime::operator>(const UnixTime& other) const {
  return utc_micros_ > other.utc_micros_;
}

inline constexpr bool UnixTime::operator<=(const UnixTime& other) const {
  return utc_micros_ <= other.utc_micros_;
}

inline constexpr bool UnixTime::operator>=(const UnixTime& other) const {
  return utc_micros_ >= other.utc_micros_;
}

inline constexpr UnixTime::operator uint64_t() const {
  return utc_micros_;
}

inline constexpr UnixTime::operator double() const {
  return utc_micros_;
}

inline constexpr uint64_t UnixTime::unixMicros() const {
  return utc_micros_;
}

inline constexpr UnixTime UnixTime::epoch() {
  return UnixTime(0);
}

inline constexpr Duration UnixTime::operator-(const UnixTime& other) const {
  return *this > other
      ? Duration::fromMicroseconds(utc_micros_ - other.utc_micros_)
      : Duration::fromMicroseconds(other.utc_micros_ - utc_micros_);
}

inline constexpr UnixTime UnixTime::operator+(const Duration& duration) const {
  return UnixTime(utc_micros_ + duration.microseconds());
}

inline constexpr UnixTime UnixTime::operator-(const Duration& duration) const {
  return UnixTime(utc_micros_ - duration.microseconds());
}

} // namespace stx
