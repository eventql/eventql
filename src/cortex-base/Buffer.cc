// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/Buffer.h>
#include <cortex-base/hash/FNV.h>
#include <cortex-base/RuntimeError.h>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <new>

namespace cortex {

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
void Buffer::setCapacity(std::size_t value) {
  if (value == 0 && capacity_) {
    free(data_);
    capacity_ = 0;
    return;
  }

  if (value > capacity_) {
    if (capacity_) {
      // pad up to CHUNK_SIZE, only if continuous regrowth)
      value = value - 1;
      value = value + CHUNK_SIZE - (value % CHUNK_SIZE);
    }
  } else if (value < capacity_) {
    // possibly adjust the actual used size
    if (value < size_) {
      size_ = value;
    }
  } else {
    // nothing changed
    return;
  }

  if (char *rp = static_cast<value_type *>(std::realloc(data_, value))) {
    // setting capacity succeed.
    data_ = rp;
    capacity_ = value;
  } else if (value == 0) {
    // freeing all Memory succeed.
    capacity_ = value;
  } else {
    // setting capacity failed, do not change anything.
    throw std::bad_alloc();
  }
}

uint32_t BufferRef::hash32() const {
  return cortex::hash::FNV<uint32_t>().hash(data(), size());
}

uint64_t BufferRef::hash64() const {
  return cortex::hash::FNV<uint64_t>().hash(data(), size());
}

std::string BufferRef::hexdump(
    const void *bytes,
    std::size_t length,
    HexDumpMode mode) {

  switch (mode) {
    case HexDumpMode::InlineNarrow:
      return hexdumpInlineNarrow(bytes, length);
    case HexDumpMode::InlineWide:
      return hexdumpInlineWide(bytes, length);
    case HexDumpMode::PrettyAscii:
      return hexdumpPrettyAscii(bytes, length);
    default:
      RAISE(InvalidArgumentError);
  }
}

const char BufferRef::hex[16] = {
  '0', '1', '2', '3', '4', '5', '6', '7',
  '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

std::string BufferRef::hexdumpInlineNarrow(const void* bytes, size_t length) {
  std::stringstream sstr;
  const unsigned char* p = (const unsigned char*) bytes;

  for (size_t i = 0; i < length; ++i) {
    sstr << hex[(*p >> 4) & 0x0F];
    sstr << hex[*p & 0x0F];
    ++p;
  }

  return sstr.str();
}

std::string BufferRef::hexdumpInlineWide(const void* bytes, size_t length) {
  std::stringstream sstr;
  const unsigned char* p = (const unsigned char*) bytes;

  for (size_t i = 0; i < length; ++i) {
    if (i) sstr << ' ';
    sstr << hex[(*p >> 4) & 0x0F];
    sstr << hex[*p & 0x0F];
    ++p;
  }

  return sstr.str();
}

std::string BufferRef::hexdumpPrettyAscii(const void* bytes, size_t length) {
  // 12 34 56 78   12 34 56 78   12 34 56 78   12 34 56 78   12345678abcdef
  const int BLOCK_SIZE = 8;
  const int BLOCK_COUNT = 2;  // 4;
  const int PLAIN_WIDTH = BLOCK_SIZE * BLOCK_COUNT;
  const int HEX_WIDTH = (BLOCK_SIZE + 1) * 3 * BLOCK_COUNT;

  std::stringstream sstr;
  char line[HEX_WIDTH + PLAIN_WIDTH + 1];
  const unsigned char* p = (const unsigned char*) bytes;

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

    sstr << line << std::endl;
  }

  return sstr.str();
}

}  // namespace cortex

