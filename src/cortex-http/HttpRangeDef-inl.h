// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#ifndef x0_HttpRangeDef_ipp
#define x0_HttpRangeDef_ipp

#include <cortex-base/Tokenizer.h>

// XXX http://tools.ietf.org/html/draft-fielding-http-p5-range-00

#include <cstdlib>

namespace cortex {
namespace http {

inline HttpRangeDef::HttpRangeDef() : ranges_() {}

inline HttpRangeDef::HttpRangeDef(const BufferRef& spec) : ranges_() {
  parse(spec);
}

/**
 * parses an HTTP/1.1 conform Range header \p value.
 * \param value the HTTP header field value retrieved from the Range header
 *field.
 *
 * The following ranges can be specified:
 * <ul>
 *    <li>explicit range, from \em first to \em last (first-last)</li>
 *    <li>explicit begin to the end of the entity (first-)</li>
 *    <li>the last N units of the entity (-last)</li>
 * </ul>
 */
inline bool HttpRangeDef::parse(const BufferRef& value) {
  // ranges-specifier = byte-ranges-specifier
  // byte-ranges-specifier = bytes-unit "=" byte-range-set
  // byte-range-set  = 1#( byte-range-spec | suffix-byte-range-spec )
  // byte-range-spec = first-byte-pos "-" [last-byte-pos]
  // first-byte-pos  = 1*DIGIT
  // last-byte-pos   = 1*DIGIT

  // suffix-byte-range-spec = "-" suffix-length
  // suffix-length = 1*DIGIT

  typedef Tokenizer<BufferRef> Tokenizer;

  for (Tokenizer spec(value, "="); !spec.end(); spec.nextToken()) {
    unitName = spec.token();

    if (unitName == "bytes") {
      for (auto& range : Tokenizer::tokenize(spec.nextToken(), ",")) {
        if (!parseRangeSpec(range)) {
          return false;
        }
      }
    }
  }

  return true;
}

inline bool HttpRangeDef::parseRangeSpec(const BufferRef& spec) {
  const char* i = const_cast<char*>(spec.cbegin());
  const char* e = spec.cend();

  if (i == e) return false;

  // parse first element
  char* eptr = const_cast<char*>(i);
  size_t a = std::isdigit(*i) ? strtoul(i, &eptr, 10) : npos;

  i = eptr;

  if (*i != '-') {
    // printf("parse error: %s (%s)\n", i, spec.c_str());
    return false;
  }

  ++i;

  if (i == e) {
    ranges_.push_back(std::make_pair(a, npos));
    return true;
  }

  // parse second element
  size_t b = strtoul(i, &eptr, 10);

  i = eptr;

  if (i != e)  // garbage at the end
    return false;

  ranges_.push_back(std::make_pair(a, b));

  return true;
}

inline void HttpRangeDef::push_back(std::size_t offset1, std::size_t offset2) {
  ranges_.push_back(std::make_pair(offset1, offset2));
}

inline void HttpRangeDef::push_back(
    const std::pair<std::size_t, std::size_t>& range) {
  ranges_.push_back(range);
}

inline std::size_t HttpRangeDef::size() const { return ranges_.size(); }

inline bool HttpRangeDef::empty() const { return !ranges_.size(); }

inline const HttpRangeDef::element_type& HttpRangeDef::operator[](
    std::size_t index) const {
  return ranges_[index];
}

inline HttpRangeDef::iterator HttpRangeDef::begin() { return ranges_.begin(); }

inline HttpRangeDef::iterator HttpRangeDef::end() { return ranges_.end(); }

inline HttpRangeDef::const_iterator HttpRangeDef::begin() const {
  return ranges_.begin();
}

inline HttpRangeDef::const_iterator HttpRangeDef::end() const {
  return ranges_.end();
}

inline std::string HttpRangeDef::str() const {
  std::stringstream sstr;
  int count = 0;

  sstr << unitName;

  for (const_iterator i = begin(), e = end(); i != e; ++i) {
    if (count++) {
      sstr << ", ";
    }

    if (i->first != npos) {
      sstr << i->first;
    }

    sstr << '-';

    if (i->second != npos) {
      sstr << i->second;
    }
  }

  return sstr.str();
}

}  // namespace http
}  // namespace cortex

// vim:syntax=cpp
#endif
