// This file is part of the "x0" project
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <cortex-http/Api.h>
#include <base/Buffer.h>
#include <map>
#include <list>
#include <deque>
#include <vector>
#include <utility>
#include <string>
#include <assert.h>
#include <stdint.h>

namespace cortex {

using namespace base;

namespace hpack {

typedef std::string HeaderFieldName;
typedef std::string HeaderFieldValue;

/**
 * A name-value pair.
 *
 * Both the name and value are treated as opaque sequences of octets.
 */
struct HeaderField {
 public:
  HeaderField() = default;
  HeaderField(HeaderField&&) = default;
  HeaderField(const HeaderField&) = default;
  HeaderField& operator=(const HeaderField&) = default;
  HeaderField(const HeaderFieldName& nam, const HeaderFieldValue& val)
      : name(nam), value(val) {}

  bool operator==(const HeaderField& other) const {
    return name == other.name && value == other.value;
  }

  bool operator!=(const HeaderField& other) const {
    //.
    return !(*this == other);
  }

 public:
  HeaderFieldName name;
  HeaderFieldValue value;
};

/**
 * Unordered group of header fields.
 *
 * A header set is an unordered group of header fields that
 * are encoded jointly. It can contain duplicate header fields.  A
 * complete set of key-value pairs contained in a HTTP request or
 * response is a header set.
 */
typedef std::list<HeaderField> HeaderSet;

/**
 * The static table (see Appendix B) is a component used
 * to associate static header fields to index values. 
 *
 * This data is ordered, read-only, always accessible,
 * and may be shared amongst
 * all encoding contexts.
 */
class CORTEX_HTTP_API StaticTable {
 public:
  StaticTable();
  ~StaticTable();

  static StaticTable* get();

 private:
  std::vector<HeaderField> entries_;
};

class ReferenceSet;

/**
 * List of headers.
 *
 * The header table (see Section 3.2) is a component used
 * to associate stored header fields to index values.
 */
class CORTEX_HTTP_API HeaderTable {
 public:
  explicit HeaderTable(size_t maxEntries);
  ~HeaderTable();

  size_t maxEntries() const { return maxEntries_; }
  void setMaxEntries(size_t value);

  bool empty() const { return entries_.empty(); }
  size_t size() const { return entries_.size(); }

  void add(const HeaderField& field);
  bool contains(const HeaderFieldName& name) const;
  bool contains(const HeaderField* name) const;
  void remove(const HeaderFieldName& name);
  void remove(const HeaderField* field);
  void clear();

  const HeaderField* entry(unsigned index) const {
    assert(index >= 1 && index <= size());
    return &entries_[index - 1];
  }

  const HeaderField& first() const {
    assert(!empty());
    return *entry(1);
  }

  const HeaderField& last() const {
    assert(!empty());
    return *entry(size());
  }

  typedef std::deque<HeaderField>::iterator iterator;
  typedef std::deque<HeaderField>::const_iterator const_iterator;

  iterator begin() { return entries_.begin(); }
  iterator end() { return entries_.end(); }

  const_iterator cbegin() const { return entries_.cbegin(); }
  const_iterator cend() const { return entries_.cend(); }

  iterator find(const HeaderFieldName& name);
  const_iterator find(const HeaderFieldName& name) const;

 private:
  friend class ReferenceSet;

  void link(ReferenceSet* ref);
  void unlink(ReferenceSet* ref);

 private:
  size_t maxEntries_;
  std::deque<HeaderField> entries_;
  std::list<ReferenceSet*> referenceSets_;
};

/**
 * used for differential encoding of a new header set.
 */
class CORTEX_HTTP_API ReferenceSet {
 public:
  explicit ReferenceSet(HeaderTable* headerTable);
  ~ReferenceSet();

  bool empty() const;
  size_t size() const;

  void add(const HeaderField* field);
  bool contains(const HeaderField* field) const;
  void remove(const HeaderField* field);

  struct iterator;

  iterator begin();
  iterator end();

  HeaderTable* headerTable() const { return headerTable_; }

 private:
  HeaderTable* headerTable_;
  std::list<const HeaderField*> references_;
};

class CORTEX_HTTP_API ReferenceSet::iterator {
 public:
  iterator();
  iterator(ReferenceSet* rs, std::list<const HeaderField*>::iterator init);
  iterator(iterator&& other) = default;
  iterator(const iterator& other) = default;
  iterator& operator=(const iterator& other) = default;

  iterator& operator++();
  bool operator==(const iterator& other) const;
  bool operator!=(const iterator& other) const;

  const HeaderField& operator*() const;

 private:
  ReferenceSet* referenceSet_;
  std::list<const HeaderField*>::iterator current_;
  std::list<const HeaderField*>::iterator end_;
};

/**
 * Helper methods for encoding header fragments. 
 */
class CORTEX_HTTP_API EncoderHelper {
 public:
  static void encodeInt(Buffer* output, uint64_t i, unsigned prefixBits);
  static void encodeIndexed(Buffer* output, unsigned index);
  static void encodeLiteral(Buffer* output, const BufferRef& name,
                            const BufferRef& value, bool indexing,
                            bool huffman);
  static void encodeIndexedLiteral(Buffer* output, unsigned index,
                                   const BufferRef& value, bool huffman);
  static void encodeTableSizeChange(Buffer* output, unsigned newSize);
};

class CORTEX_HTTP_API Encoder : private EncoderHelper {
 public:
  Encoder();
  ~Encoder();

  void encode(const HeaderSet& headerBlock);
};

/**
 * Helper methods for decoding header fragments. 
 */
class CORTEX_HTTP_API DecoderHelper {
 public:
  static uint64_t decodeInt(const BufferRef& input, unsigned prefixBits,
                            unsigned* bytesConsumed);
};

class CORTEX_HTTP_API Decoder : private DecoderHelper {
 public:
  Decoder();
  ~Decoder();

  void decode(const BufferRef& headerBlock);
};

}  // namespace hpack
}  // namespace cortex

// vim:ts=2:sw=2
