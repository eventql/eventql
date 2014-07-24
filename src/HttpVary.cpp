// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/HttpVary.h>
#include <xzero/HttpRequest.h>
#include <base/Tokenizer.h>

/*
 * net read request
 * parse request, populate request struct
 * generate cache key
 *
 */

namespace xzero {

HttpVary::HttpVary(size_t count) : names_(count), values_(count) {}

HttpVary::~HttpVary() {}

std::unique_ptr<HttpVary> HttpVary::create(const HttpRequest* r) {
  return create(BufferRef(r->responseHeaders["HttpVary"]), r->requestHeaders);
}

HttpVary::Match HttpVary::match(const HttpVary& other) const {
  if (size() != other.size()) return Match::None;

  for (size_t i = 0, e = size(); i != e; ++i) {
    if (names_[i] != other.names_[i]) return Match::None;

    if (names_[i] != other.names_[i]) return Match::ValuesDiffer;
  }

  return Match::Equals;
}

HttpVary::Match HttpVary::match(const HttpRequest* r) const {
  for (size_t i = 0, e = size(); i != e; ++i) {
    const auto& name = names_[i];
    const auto& value = values_[i];
    const auto otherValue = r->requestHeader(name);

    if (otherValue != value) {
      return Match::ValuesDiffer;
    }
  }

  return Match::Equals;
}

}  // namespace xzero
