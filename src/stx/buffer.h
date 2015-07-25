/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_UTIL_BUFFER_H_
#define _FNORD_UTIL_BUFFER_H_
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include "stx/stdtypes.h"
#include "stx/autoref.h"
#include "stx/VFSFile.h"

namespace stx {

class Buffer : public VFSFile {
public:
  static const size_t npos = -1;

  Buffer();
  Buffer(const void* initial_data, size_t initial_size);
  Buffer(size_t initial_size);
  Buffer(const String& string);
  Buffer(const Buffer& copy);
  Buffer(Buffer&& move);
  Buffer& operator=(const Buffer& copy);
  Buffer& operator=(Buffer&& move);
  ~Buffer();

  bool operator==(const Buffer& buf) const;
  bool operator==(const char* str) const;
  bool operator==(const std::string& str) const;

  void append(const void* data, size_t size);
  void append(const String& string);
  void append(const Buffer& buffer);
  void append(char chr);
  void clear();
  void truncate(size_t size);

  void* data() const;
  char charAt(size_t pos) const;
  size_t find(char chr) const;

  /**
   * Return the logical size of the buffer
   */
  size_t size() const;

  /**
   * Return the actual size of the backing malloc, this may be larger than the
   * value returned by size
   */
  size_t allocSize() const;
  size_t capacity() const;

  /**
   * Reserve "size" new bytes of memory for future use. This method will not
   * change the logical size of the buffer (as returned by a call to size).
   */
  void reserve(size_t size);

  std::string toString() const;

  void setMark(size_t mark);
  size_t mark() const;

protected:
  void* data_;
  size_t size_;
  size_t alloc_;
  size_t mark_;
};

typedef RefPtr<Buffer> BufferRef;

}
#endif
