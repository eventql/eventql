// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#ifndef x0_HttpRangeDef_h
#define x0_HttpRangeDef_h

#include <cortex-http/Api.h>
#include <cortex-base/Buffer.h>

#include <vector>
#include <sstream>

namespace cortex {
namespace http {

//! \addtogroup http
//@{

/**
 * \brief represents a Range-header field with high-level access.
 */
class CORTEX_HTTP_API HttpRangeDef {
 public:
  typedef std::pair<std::size_t, std::size_t> element_type;

  /** internally used range vector type. */
  typedef std::vector<element_type> vector_type;

  /** range iterator. */
  typedef vector_type::iterator iterator;

  typedef vector_type::const_iterator const_iterator;

  /** represents an unspecified range item of a range-pair component.
   *
   * Example ranges for file of 1000 bytes:
   * <ul>
   *   <li>(npos, 500) - last 500 bytes</li>
   *   <li>(9500, 999) - from 9500 to 999 (also: last 500 bytes in this
   *case)</li>
   *   <li>(9500, npos) - bytes from 9500 to the end of entity. (also: last 500
   *bytes in this case)</li>
   * <ul>
   */
  enum { npos = std::size_t(-1) };

 private:
  vector_type ranges_;

 public:
  HttpRangeDef();
  explicit HttpRangeDef(const BufferRef& spec);

  BufferRef unitName;

  bool parse(const BufferRef& value);

  /// pushes a new range to the list of ranges
  void push_back(std::size_t offset1, std::size_t offset2);

  /// pushes a new range to the list of ranges
  void push_back(const std::pair<std::size_t, std::size_t>& range);

  std::size_t size() const;

  bool empty() const;

  /** retrieves the range element at given \p index. */
  const element_type& operator[](std::size_t index) const;

  /** iterator pointing to the first range element. */
  const_iterator begin() const;

  /** iterator representing the end of this range vector. */
  const_iterator end() const;

  /** iterator pointing to the first range element. */
  iterator begin();

  /** iterator representing the end of this range vector. */
  iterator end();

  /** retrieves string representation of this range. */
  std::string str() const;

 private:
  inline bool parseRangeSpec(const BufferRef& spec);
};

//@}

}  // namespace http
}  // namespace cortex

#include <cortex-http/HttpRangeDef-inl.h>

#endif
