// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <cortex-http/HeaderField.h>
#include <cortex-base/StringUtil.h>
#include <cortex-base/Buffer.h>

namespace cortex {
namespace http {

HeaderField::HeaderField(const std::string& name, const std::string& value)
  : name_(name), value_(value) {
}

bool HeaderField::operator==(const HeaderField& other) const {
  return iequals(name(), other.name()) && iequals(value(), other.value());
}

bool HeaderField::operator!=(const HeaderField& other) const {
  return !(*this == other);
}

std::string inspect(const HeaderField& field) {
  return StringUtil::format("HeaderField(\"$0\", \"$1\")",
                            field.name(),
                            field.value());
}

} // namespace http
} // namespace cortex
