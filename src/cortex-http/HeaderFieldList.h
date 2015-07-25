// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <cortex-http/Api.h>
#include <cortex-http/HeaderField.h>
#include <string>
#include <list>

namespace cortex {
namespace http {

/**
 * Represents a list of headers (key/value pairs) for an HTTP message.
 */
class CORTEX_HTTP_API HeaderFieldList {
 public:
  HeaderFieldList() = default;
  HeaderFieldList(HeaderFieldList&&) = default;
  HeaderFieldList(const HeaderFieldList&) = default;
  HeaderFieldList& operator=(HeaderFieldList&&) = default;
  HeaderFieldList& operator=(const HeaderFieldList&) = default;
  HeaderFieldList(const std::initializer_list<std::pair<std::string, std::string>>& init);

  void push_back(const std::string& name, const std::string& value);
  void overwrite(const std::string& name, const std::string& value);
  void append(const std::string& name, const std::string& value,
              const std::string& delim = "");
  void remove(const std::string& name);

  bool empty() const;
  size_t size() const;
  bool contains(const std::string& name) const;
  bool contains(const std::string& name, const std::string& value) const;
  const std::string& get(const std::string& name) const;
  const std::string& operator[](const std::string& name) const;

  typedef std::list<HeaderField>::iterator iterator;
  typedef std::list<HeaderField>::const_iterator const_iterator;

  iterator begin() { return entries_.begin(); }
  iterator end() { return entries_.end(); }

  const_iterator begin() const { return entries_.cbegin(); }
  const_iterator end() const { return entries_.cend(); }

  const_iterator cbegin() const { return entries_.cbegin(); }
  const_iterator cend() const { return entries_.cend(); }

  /**
   * Completely removes all entries from this header list.
   */
  void reset();

  void swap(HeaderFieldList& other);

 private:
  std::list<HeaderField> entries_;
};

inline bool HeaderFieldList::empty() const {
  return entries_.empty();
}

inline size_t HeaderFieldList::size() const {
  return entries_.size();
}

inline void HeaderFieldList::swap(HeaderFieldList& other) {
  entries_.swap(other.entries_);
}

}  // namespace http
}  // namespace cortex
