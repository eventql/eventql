// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <cortex-http/Api.h>
#include <string>

namespace cortex {
namespace http {

/**
 * Represents a single HTTP message header name/value pair.
 */
class CORTEX_HTTP_API HeaderField {
 public:
  HeaderField() = default;
  HeaderField(HeaderField&&) = default;
  HeaderField(const HeaderField&) = default;
  HeaderField& operator=(HeaderField&&) = default;
  HeaderField& operator=(const HeaderField&) = default;
  HeaderField(const std::string& name, const std::string& value);

  const std::string& name() const { return name_; }
  void setName(const std::string& name) { name_ = name; }

  const std::string& value() const { return value_; }
  void setValue(const std::string& value) { value_ = value; }

  void appendValue(const std::string& value, const std::string& delim = "") {
    if (value_.empty()) {
      value_ = value;
    } else {
      value_ += delim;
      value_ += value;
    }
  }

  /** Performs an case-insensitive compare on name and value for equality. */
  bool operator==(const HeaderField& other) const;

  /** Performs an case-insensitive compare on name and value for inequality. */
  bool operator!=(const HeaderField& other) const;

 private:
  std::string name_;
  std::string value_;
};

CORTEX_HTTP_API std::string inspect(const HeaderField& field);

}  // namespace http
}  // namespace cortex
