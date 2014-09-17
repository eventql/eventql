// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/Buffer.h>
#include <cstdio>

namespace xzero {

/*! changes the capacity of the underlying buffer, possibly reallocating into
 *more or less bytes reserved.
 *
 * This method either increases or decreases the reserved memory.
 * If it increases the size, and it is not the first capacity change, it will be
 *aligned
 * to \p CHUNK_SIZE bytes, otherwise the exact size will be reserved.
 * If the requested size is lower than the current capacity, then the underlying
 *storage
 * will be redused to exactly this size and the actually used buffer size is cut
 *down
 * to the available capacity if it would exceed the capacity otherwise.
 * Reduzing the capacity to zero implicitely means to free up all storage.
 * If the requested size is equal to the current one, nothing happens.
 *
 * \param value the length in bytes you want to change the underlying storage
 *to.
 *
 * \retval true the requested capacity is available. go ahead.
 * \retval false could not change capacity. take caution!
 */
bool Buffer::setCapacity(std::size_t value) {
  if (value == 0 && capacity_) {
    free(data_);
    capacity_ = 0;
    return true;
  }

  if (value > capacity_) {
    if (capacity_) {
      // pad up to CHUNK_SIZE, only if continuous regrowth)
      value = value - 1;
      value = value + CHUNK_SIZE - (value % CHUNK_SIZE);
    }
  } else if (value < capacity_) {
    // possibly adjust the actual used size
    if (value < size_) size_ = value;
  } else
    // nothing changed
    return true;

  if (char *rp = static_cast<value_type *>(std::realloc(data_, value))) {
    // setting capacity succeed.
    data_ = rp;
    capacity_ = value;
    return true;
  } else if (value == 0) {
    // freeing all Memory succeed.
    capacity_ = value;
    return true;
  } else {
    // setting capacity failed, do not change anything.
    return false;
  }
}

void BufferRef::dump(const void *bytes, std::size_t length,
                     const char *description) {
  static char hex[] = "0123456789ABCDEF";
  const int BLOCK_SIZE = 8;
  const int BLOCK_COUNT = 2;  // 4;

  // 12 34 56 78   12 34 56 78   12 34 56 78   12 34 56 78   12345678abcdef
  const int HEX_WIDTH = (BLOCK_SIZE + 1) * 3 * BLOCK_COUNT;
  const int PLAIN_WIDTH = BLOCK_SIZE * BLOCK_COUNT;
  char line[HEX_WIDTH + PLAIN_WIDTH + 1];

  const char *p = (const char *)bytes;

  if (description && *description)
    std::printf("%s (%zu bytes):\n", description, length);
  else
    std::printf("Memory dump (%zu bytes):\n", length);

  while (length > 0) {
    char *u = line;
    char *v = u + HEX_WIDTH;

    int blockNr = 0;
    for (; blockNr < BLOCK_COUNT && length > 0; ++blockNr) {
      // block
      int i = 0;
      for (; i < BLOCK_SIZE && length > 0; ++i) {
        // hex frame
        *u++ = hex[(*p >> 4) & 0x0F];
        *u++ = hex[*p & 0x0F];
        *u++ = ' ';  // byte separator

        // plain text frame
        *v++ = std::isprint(*p) ? *p : '.';

        ++p;
        --length;
      }

      for (; i < BLOCK_SIZE; ++i) {
        *u++ = ' ';
        *u++ = ' ';
        *u++ = ' ';
      }

      // block separator
      *u++ = ' ';
      *u++ = ' ';
      *u++ = ' ';
    }

    for (; blockNr < BLOCK_COUNT; ++blockNr) {
      for (int i = 0; i < BLOCK_SIZE; ++i) {
        *u++ = ' ';
        *u++ = ' ';
        *u++ = ' ';
      }
      *u++ = ' ';
      *u++ = ' ';
      *u++ = ' ';
    }

    // EOS
    *v = '\0';

    std::printf("%s\n", line);
  }
}

}  // namespace xzero
