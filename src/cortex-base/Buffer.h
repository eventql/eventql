// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <cortex-base/Api.h>
#include <cortex-base/sysconfig.h>
#include <cstddef>
#include <climits>
#include <cstring>
#include <cstdlib>
#include <cstdarg>
#include <cassert>
#include <string>
#include <stdexcept>
#include <new>

namespace cortex {

//! \addtogroup base
//@{

template <typename>
class BufferBase;
template <void (*ensure)(void*, size_t)>
class MutableBuffer;
class BufferRef;
class BufferSlice;
class FixedBuffer;
class Buffer;

// {{{ BufferTraits
template <typename T>
struct BufferTraits;

template <>
struct BufferTraits<char*> {
  typedef char value_type;
  typedef char& reference_type;
  typedef const char& const_reference_type;
  typedef char* pointer_type;
  typedef char* iterator;
  typedef const char* const_iterator;
  typedef char* data_type;
};

struct BufferOffset {  // {{{
  mutable Buffer* buffer_;
  size_t offset_;

  BufferOffset() : buffer_(nullptr), offset_(0) {}

  BufferOffset(Buffer* buffer, size_t offset)
      : buffer_(buffer), offset_(offset) {}

  BufferOffset(const BufferOffset& v)
      : buffer_(v.buffer_), offset_(v.offset_) {}

  BufferOffset(BufferOffset&& v) : buffer_(v.buffer_), offset_(v.offset_) {
    v.buffer_ = nullptr;
    v.offset_ = 0;
  }

  BufferOffset& operator=(const BufferOffset& v) {
    buffer_ = v.buffer_;
    offset_ = v.offset_;

    return *this;
  }

  Buffer* buffer() const { return buffer_; }
  size_t offset() const { return offset_; }

  operator const char*() const;
  operator char*();
};  // }}}

template <>
struct BufferTraits<Buffer> {
  typedef char value_type;
  typedef char& reference_type;
  typedef const char& const_reference_type;
  typedef char* pointer_type;
  typedef char* iterator;
  typedef const char* const_iterator;
  typedef BufferOffset data_type;
};
// }}}
// {{{ BufferBase<T>
enum class HexDumpMode {
  InlineWide,
  InlineNarrow,
  PrettyAscii
};

template <typename T>
class CORTEX_API BufferBase {
 public:
  typedef typename BufferTraits<T>::value_type value_type;
  typedef typename BufferTraits<T>::reference_type reference_type;
  typedef typename BufferTraits<T>::const_reference_type const_reference_type;
  typedef typename BufferTraits<T>::pointer_type pointer_type;
  typedef typename BufferTraits<T>::iterator iterator;
  typedef typename BufferTraits<T>::const_iterator const_iterator;
  typedef typename BufferTraits<T>::data_type data_type;

  enum { npos = size_t(-1) };

 protected:
  data_type data_;
  size_t size_;

 public:
  constexpr BufferBase() : data_(), size_(0) {}
  constexpr BufferBase(data_type data, size_t size) : data_(data), size_(size) {}
  constexpr BufferBase(const BufferBase<T>& v) : data_(v.data_), size_(v.size_) {}

  // properties
  pointer_type data() { return data_; }
  constexpr const pointer_type data() const {
    return const_cast<BufferBase<T>*>(this)->data_;
  }
  reference_type at(size_t offset) { return data()[offset]; }
  constexpr const reference_type at(size_t offset) const { return data()[offset]; }
  constexpr size_t size() const { return size_; }

  constexpr bool empty() const { return size_ == 0; }
  constexpr operator bool() const { return size_ != 0; }
  constexpr bool operator!() const { return size_ == 0; }

  void clear() { size_ = 0; }

  // iterator access
  constexpr iterator begin() const { return const_cast<BufferBase<T>*>(this)->data_; }
  constexpr iterator end() const {
    return const_cast<BufferBase<T>*>(this)->data_ + size_;
  }

  constexpr const_iterator cbegin() const { return data(); }
  constexpr const_iterator cend() const { return data() + size(); }

  // find
  template <typename PodType, size_t N>
  size_t find(PodType (&value)[N], size_t offset = 0) const;
  size_t find(const value_type* value, size_t offset = 0) const;
  size_t find(const BufferRef& value, size_t offset = 0) const;
  size_t find(const std::string& value, size_t offset = 0) const;
  size_t find(value_type value, size_t offset = 0) const;

  // rfind
  template <typename PodType, size_t N>
  size_t rfind(PodType (&value)[N]) const;
  size_t rfind(const value_type* value) const;
  size_t rfind(const BufferRef& value) const;
  size_t rfind(value_type value) const;
  size_t rfind(value_type value, size_t offset) const;

  // contains
  bool contains(const BufferRef& ref) const;

  // split
  std::pair<BufferRef, BufferRef> split(char delimiter) const;
  std::pair<BufferRef, BufferRef> split(const value_type* delimiter) const;

  // begins / ibegins
  bool begins(const BufferRef& value) const;
  bool begins(const value_type* value) const;
  bool begins(const std::string& value) const;
  bool begins(value_type value) const;

  bool ibegins(const BufferRef& value) const;
  bool ibegins(const value_type* value) const;
  bool ibegins(const std::string& value) const;
  bool ibegins(value_type value) const;

  // ends / iends
  bool ends(const BufferRef& value) const;
  bool ends(const value_type* value) const;
  bool ends(const std::string& value) const;
  bool ends(value_type value) const;

  bool iends(const BufferRef& value) const;
  bool iends(const value_type* value) const;
  bool iends(const std::string& value) const;
  bool iends(value_type value) const;

  // sub
  BufferRef ref(size_t offset = 0) const;
  BufferRef ref(size_t offset, size_t size) const;

  // mutation
  BufferRef chomp() const;
  BufferRef trim() const;
  Buffer replaceAll(char src, char dst) const;
  Buffer replaceAll(const char* src, const char* dst) const;

  // STL string
  std::string str() const;
  std::string substr(size_t offset) const;
  std::string substr(size_t offset, size_t count) const;

  // casts
  template <typename U>
  U hex() const;

  bool toBool() const;
  int toInt() const;
  double toDouble() const;
  float toFloat() const;

  std::string hexdump(HexDumpMode mode = HexDumpMode::InlineWide) const;
};

template <typename T>
bool equals(const BufferBase<T>& a, const BufferBase<T>& b);
template <typename T>
bool equals(const BufferBase<T>& a, const std::string& b);
template <typename T, typename PodType, std::size_t N>
bool equals(const BufferBase<T>& a, PodType (&b)[N]);
template <typename T>
bool equals(const std::string& a, const std::string& b);

template <typename T>
bool iequals(const BufferBase<T>& a, const BufferBase<T>& b);
template <typename T>
bool iequals(const BufferBase<T>& a, const std::string& b);
template <typename T, typename PodType, std::size_t N>
bool iequals(const BufferBase<T>& a, PodType (&b)[N]);
template <typename T>
bool iequals(const std::string& a, const std::string& b);

template <typename T>
bool operator==(const BufferBase<T>& a, const BufferBase<T>& b);
template <typename T>
bool operator==(const BufferBase<T>& a, const std::string& b);
template <typename T, typename PodType, std::size_t N>
bool operator==(const BufferBase<T>& a, PodType (&b)[N]);
template <typename T, typename PodType, std::size_t N>
bool operator==(PodType (&b)[N], const BufferBase<T>& a);

template <typename T>
bool operator!=(const BufferBase<T>& a, const BufferBase<T>& b) {
  return !(a == b);
}
template <typename T>
bool operator!=(const BufferBase<T>& a, const std::string& b) {
  return !(a == b);
}
template <typename T, typename PodType, std::size_t N>
bool operator!=(const BufferBase<T>& a, PodType (&b)[N]) {
  return !(a == b);
}
template <typename T, typename PodType, std::size_t N>
bool operator!=(PodType (&b)[N], const BufferBase<T>& a) {
  return !(a == b);
}
// }}}
// {{{ BufferRef
/**
 * Represents any arbitrary read-only byte-buffer region.
 *
 * The underlying buffer is not managed by this class, that means, the
 * buffer reference owner has to make sure the underlying buffer exists
 * during the whole lifetime of this buffer reference.
 */
class CORTEX_API BufferRef : public BufferBase<char*> {
 public:
  /** Initializes an empty buffer reference. */
  constexpr BufferRef() : BufferBase<char*>() {}

  /**
   * Initializes buffer reference with given vector.
   *
   * @param data buffer start
   * @param size number of bytes this buffer holds
   */
  constexpr BufferRef(const char* data, size_t size)
      : BufferBase<char*>((data_type)data, size) {}

  /**
   * Initializes buffer reference with pointer to given std::string.
   */
  explicit constexpr BufferRef(const std::string& v)
      : BufferBase<char*>((data_type)v.data(), v.size()) {}

  constexpr BufferRef(const BufferRef& v) : BufferBase<char*>(v) {}

  /**
   * Initializes buffer reference with given POD string literal.
   */
  template <typename PodType, size_t N>
  constexpr BufferRef(PodType (&value)[N])
      : BufferBase<char*>((data_type)value, N - 1) {}

  BufferRef& operator=(const BufferRef& v);

  void swap(BufferRef& other);

  /**
   * Random access operator.
   */
  constexpr const_reference_type operator[](size_t index) const;

  /**
   * Shifts the (left) start pointer by given @p offset bytes to the left.
   *
   * Thus, increasing the size of the buffer, if @p offset is positive.
   *
   * @p offset number of bytes to shift the left start-pointer to the right.
   */
  void shl(ssize_t offset = 1);

  /**
   * Shifts the (right) end pointer by given @p offset bytes to the right.
   *
   * @p offset number of bytes to shift the right end-pointer to the right.
   */
  void shr(ssize_t offset = 1);

  using BufferBase<char*>::hexdump;
  static std::string hexdump(
      const void* bytes, std::size_t length,
      HexDumpMode mode = HexDumpMode::InlineWide);

  class CORTEX_API reverse_iterator {  // {{{
   private:
    BufferRef* buf_;
    int cur_;

   public:
    reverse_iterator(BufferRef* r, int cur) : buf_(r), cur_(cur) {}
    reverse_iterator& operator++() {
      if (cur_ >= 0) {
        --cur_;
      }
      return *this;
    }
    const char& operator*() const {
      assert(cur_ >= 0 && cur_ < (int)buf_->size());
      return (*buf_)[cur_];
    }
    bool operator==(const reverse_iterator& other) const {
      return buf_ == other.buf_ && cur_ == other.cur_;
    }
    bool operator!=(const reverse_iterator& other) const {
      return !(*this == other);
    }
  };  // }}}

  reverse_iterator rbegin() const {
    return reverse_iterator((BufferRef*)this, size() - 1);
  }

  reverse_iterator rend() const {
    return reverse_iterator((BufferRef*)this, -1);
  }

  uint32_t hash32() const;
  uint64_t hash64() const;
  uint64_t hash() const { return hash64(); }

 private:
  static std::string hexdumpInlineNarrow(const void* bytes, size_t length);
  static std::string hexdumpInlineWide(const void* bytes, size_t length);
  static std::string hexdumpPrettyAscii(const void* bytes, size_t length);

  static const char hex[16];
};
// }}}
// {{{ MutableBuffer

inline void immutableEnsure(void* self, size_t size);
inline void mutableEnsure(void* self, size_t size);

/**
 * \brief Fixed size unmanaged mutable buffer.
 *
 * @param ensure function invoked to ensure that enough space is available to
 *write to.
 */
template <void (*ensure)(void*, size_t)>
class CORTEX_API MutableBuffer : public BufferRef {
 protected:
  size_t capacity_;

 public:
  MutableBuffer();
  MutableBuffer(const MutableBuffer& other);
  MutableBuffer(char* value, size_t capacity, size_t size);
  MutableBuffer(MutableBuffer&& v);

  bool resize(size_t value);

  size_t capacity() const;
  bool operator!() const;

  void reserve(size_t value);

  // buffer builders
  void push_back(value_type value);
  void push_back(value_type value, size_t count);
  void push_back(int value);
  void push_back(long value);
  void push_back(long long value);
  void push_back(unsigned value);
  void push_back(unsigned long value);
  void push_back(unsigned long long value);
  void push_back(const BufferRef& value);
  void push_back(const BufferRef& value, size_t offset, size_t length);
  void push_back(const std::string& value);
  void push_back(const void* value, size_t size);
  template <typename PodType, size_t N>
  void push_back(PodType (&value)[N]);

  size_t vprintf(const char* fmt, va_list args);

  size_t printf(const char* fmt);

  template <typename... Args>
  size_t printf(const char* fmt, Args... args);

  // random access
  reference_type operator[](size_t index);
  const_reference_type operator[](size_t index) const;

  const value_type* c_str() const;
};
// }}}
// {{{ FixedBuffer
/**
 * Fixed-width unmanaged mutable buffer.
 *
 * Use this class in order to have all features of a mutable buffer
 * on an externally managed memory region.
 *
 * @code
 *    char raw[32];
 *    FixedBuffer obj(raw, sizeof(raw), 0);
 *    obj.push_back("Hello, World");
 *    printf("result: '%s'\n", obj.c_str());
 * @endcode
 */
class CORTEX_API FixedBuffer : public MutableBuffer<immutableEnsure> {
 public:
  FixedBuffer();
  FixedBuffer(const FixedBuffer& v);
  FixedBuffer(FixedBuffer&& v);
  FixedBuffer(char* data, size_t capacity, size_t size);

  FixedBuffer& operator=(const FixedBuffer& v);
  FixedBuffer& operator=(const Buffer& v);
  FixedBuffer& operator=(const BufferRef& v);
  FixedBuffer& operator=(const std::string& v);
  FixedBuffer& operator=(const value_type* v);
};
// }}}
// {{{ Buffer
/**
 * \brief defines a memory buffer construction and access API.
 *
 * This class should be used when sequentially creating and reading parts from
 *it is the main goal
 * of some certain linear information to share.
 */
class CORTEX_API Buffer : public MutableBuffer<mutableEnsure> {
 public:
  enum { CHUNK_SIZE = 4096 };

 public:
  Buffer();
  explicit Buffer(size_t capacity);
  explicit Buffer(const char* value);
  explicit Buffer(const BufferRef& v);
  template <typename PodType, size_t N>
  Buffer(PodType (&value)[N]);
  Buffer(const std::string& v);
  Buffer(const Buffer& v);
  Buffer(const BufferRef& v, size_t offset, size_t size);
  Buffer(const value_type* value, size_t size);
  Buffer(Buffer&& v);
  ~Buffer();

  Buffer& operator=(Buffer&& v);
  Buffer& operator=(const Buffer& v);
  Buffer& operator=(const BufferRef& v);
  Buffer& operator=(const std::string& v);
  Buffer& operator=(const value_type* v);

  void swap(Buffer& other);

  size_t mark() const CORTEX_NOEXCEPT;
  void setMark(size_t value);

  BufferSlice slice(size_t offset = 0) const;
  BufferSlice slice(size_t offset, size_t size) const;

  void setCapacity(size_t value);

  //	operator bool () const;
  //	bool operator!() const;

  static Buffer fromCopy(const value_type* data, size_t count);

 private:
  size_t mark_;
};
// }}}
// {{{ BufferSlice
/** Holds a reference to a slice (region) of a managed mutable buffer.
 */
class CORTEX_API BufferSlice : public BufferBase<Buffer> {
 public:
  BufferSlice();
  BufferSlice(Buffer& buffer, size_t offset, size_t _size);
  BufferSlice(const BufferSlice& v);

  BufferSlice& operator=(const BufferSlice& v);

  BufferSlice slice(size_t offset = 0) const;
  BufferSlice slice(size_t offset, size_t size) const;

  void shl(ssize_t offset = 1);
  void shr(ssize_t offset = 1);

  // random access
  reference_type operator[](size_t offset) { return data()[offset]; }
  const_reference_type operator[](size_t offset) const {
    return data()[offset];
  }

  Buffer* buffer() const { return data_.buffer(); }
};
// }}}
// {{{ free functions API
CORTEX_API Buffer& operator<<(Buffer& b, Buffer::value_type v);
CORTEX_API Buffer& operator<<(Buffer& b, int v);
CORTEX_API Buffer& operator<<(Buffer& b, long v);
CORTEX_API Buffer& operator<<(Buffer& b, long long v);
CORTEX_API Buffer& operator<<(Buffer& b, unsigned v);
CORTEX_API Buffer& operator<<(Buffer& b, unsigned long v);
CORTEX_API Buffer& operator<<(Buffer& b, unsigned long long v);
CORTEX_API Buffer& operator<<(Buffer& b, const Buffer& v);
CORTEX_API Buffer& operator<<(Buffer& b, const BufferRef& v);
CORTEX_API Buffer& operator<<(Buffer& b, const std::string& v);
CORTEX_API Buffer& operator<<(Buffer& b, typename Buffer::value_type* v);

template <typename PodType, size_t N>
CORTEX_API Buffer& operator<<(Buffer& b, PodType (&v)[N]);

CORTEX_API Buffer& operator+=(Buffer& b, const BufferRef& v);
CORTEX_API Buffer& operator+=(Buffer& b, const std::string& v);
CORTEX_API Buffer& operator+=(Buffer& b, Buffer::value_type v);

template <typename PodType, size_t N>
CORTEX_API Buffer& operator+=(Buffer& b, PodType (&v)[N]);
// }}}
// {{{ free functions (concatenation) API
CORTEX_API Buffer operator+(const BufferRef& a, const BufferRef& b);
// }}}
//@}

// {{{ BufferTraits helper impl
inline BufferOffset::operator char*() {
  assert(buffer_ != nullptr &&
         "BufferSlice must not be empty when accessing data.");
  return buffer_->data() + offset_;
}

inline BufferOffset::operator const char*() const {
  assert(buffer_ != nullptr &&
         "BufferSlice must not be empty when accessing data.");
  return const_cast<BufferOffset*>(this)->buffer_->data() + offset_;
}
// }}}
// {{{ BufferBase<T> impl
template <typename T>
inline bool BufferBase<T>::contains(const BufferRef& ref) const {
  return find(ref) != Buffer::npos;
}

template <typename T>
inline size_t BufferBase<T>::find(const value_type* value,
                                  size_t offset) const {
  const int value_length = strlen(value);

  if (value_length == 1)
    return find(*value, offset);

  const char* i = cbegin() + offset;
  const char* e = cend();

  if (offset + value_length > size())
    return npos;

  while (i != e) {
    if (*i == *value) {
      const char* p = i + 1; // points to the second search-byte (data)
      const char* q = value + 1; // points to the second search-byte (pattern)
      const char* qe = i + value_length; // EOS (pattern)

      while (*p == *q && p != qe) {
        ++p;
        ++q;
      }

      if (p == qe) {
        return i - data();
      }
    }
    ++i;
  }

  return npos;
}

template <typename T>
inline size_t BufferBase<T>::find(value_type value, size_t offset) const {
  if (const char* p = (const char*)memchr((const void*)(data() + offset), value,
                                          size() - offset))
    return p - data();

  return npos;
}

template <typename T>
inline size_t BufferBase<T>::find(const BufferRef& buf, size_t offset) const {
  const int value_length = buf.size();

  if (value_length == 1)
    return find(buf[0], offset);

  const char* value = buf.data();
  const char* i = cbegin() + offset;
  const char* e = cend();

  if (offset + value_length > size())
    return npos;

  while (i != e) {
    if (*i == *value) {
      const char* p = i + 1;
      const char* q = value + 1;
      const char* qe = i + value_length;

      while (*p == *q && p != qe) {
        ++p;
        ++q;
      }

      if (p == qe) {
        return i - data();
      }
    }
    ++i;
  }

  return npos;
}

template <typename T>
inline size_t BufferBase<T>::find(
    const std::string& value, size_t offset) const {
  return find(BufferRef(value.data(), value.size()), offset);
}

template <typename T>
template <typename PodType, size_t N>
inline size_t BufferBase<T>::find(PodType (&value)[N], size_t offset) const {
  const char* i = data() + offset;
  const char* e = i + size() - offset;
  const size_t value_length = N - 1;

  if (offset + value_length > size())
    return npos;

  while (i != e) {
    if (*i == *value) {
      const char* p = i + 1;
      const char* q = value + 1;
      const char* qe = i + value_length;

      while (*p == *q && p != qe) {
        ++p;
        ++q;
      }

      if (p == qe) {
        return i - data();
      }
    }
    ++i;
  }

  return npos;
}

template <typename T>
inline size_t BufferBase<T>::rfind(const value_type* value) const {
  //! \todo implementation
  assert(0 && "not implemented");
  return npos;
}

template <typename T>
inline size_t BufferBase<T>::rfind(value_type value) const {
  return rfind(value, size() - 1);
}

template <typename T>
inline size_t BufferBase<T>::rfind(value_type value, size_t offset) const {
  if (empty()) return npos;

  const char* p = data();
  const char* q = p + offset;

  for (;;) {
    if (*q == value) return q - p;

    if (p == q) break;

    --q;
  }

  return npos;
}

template <typename T>
template <typename PodType, size_t N>
size_t BufferBase<T>::rfind(PodType (&value)[N]) const {
  if (empty()) return npos;

  if (size() < N - 1) return npos;

  const char* i = end();
  const char* e = begin() + (N - 1);

  while (i != e) {
    if (*i == value[N - 1]) {
      bool found = true;

      for (int n = 0; n < N - 2; ++n) {
        if (i[n] != value[n]) {
          found = !found;
          break;
        }
      }

      if (found) {
        return i - begin();
      }
    }
    --i;
  }

  return npos;
}

template <typename T>
std::pair<BufferRef, BufferRef> BufferBase<T>::split(char delimiter) const {
  size_t i = find(delimiter);
  if (i == npos)
    return std::make_pair(ref(), BufferRef());
  else
    return std::make_pair(ref(0, i), ref(i + 1, npos));
}

template <typename T>
std::pair<BufferRef, BufferRef> BufferBase<T>::split(
    const value_type* delimiter) const {
  size_t i = find(delimiter);
  if (i == npos)
    return std::make_pair(ref(), BufferRef());
  else
    return std::make_pair(ref(0, i), ref(i + strlen(delimiter), npos));
}

template <typename T>
inline bool BufferBase<T>::begins(const BufferRef& value) const {
  return value.size() <= size() &&
         std::memcmp(data(), value.data(), value.size()) == 0;
}

template <typename T>
inline bool BufferBase<T>::begins(const value_type* value) const {
  if (!value) return true;

  size_t len = std::strlen(value);
  return len <= size() && std::memcmp(data(), value, len) == 0;
}

template <typename T>
inline bool BufferBase<T>::begins(const std::string& value) const {
  if (value.empty()) return true;

  return value.size() <= size() &&
         std::memcmp(data(), value.data(), value.size()) == 0;
}

template <typename T>
inline bool BufferBase<T>::begins(value_type value) const {
  return size() >= 1 && data()[0] == value;
}

template <typename T>
inline bool BufferBase<T>::ibegins(const value_type* value) const {
  if (!value) return true;

  size_t len = std::strlen(value);
  return len <= size() && strncasecmp(data(), value, len) == 0;
}

template <typename T>
inline bool BufferBase<T>::ends(const BufferRef& value) const {
  if (value.empty()) return true;

  size_t valueLength = value.size();

  if (size() < valueLength) return false;

  return std::memcmp(data() + size() - valueLength, value.data(),
                     valueLength) == 0;
}

template <typename T>
inline bool BufferBase<T>::ends(const std::string& value) const {
  if (value.empty()) return true;

  size_t valueLength = value.size();

  if (size() < valueLength) return false;

  return std::memcmp(data() + size() - valueLength, value.data(),
                     valueLength) == 0;
}

template <typename T>
inline bool BufferBase<T>::ends(value_type value) const {
  return size() >= 1 && data()[size() - 1] == value;
}

template <typename T>
inline bool BufferBase<T>::ends(const value_type* value) const {
  if (!value) return true;

  size_t valueLength = std::strlen(value);

  if (size() < valueLength) return false;

  return std::memcmp(data() + size() - valueLength, value, valueLength) == 0;
}

template <>
inline BufferRef BufferBase<char*>::ref(size_t offset) const {
  assert(offset <= size());
  return BufferRef(data() + offset, size() - offset);
}

template <>
inline BufferRef BufferBase<char*>::ref(size_t offset, size_t count) const {
  assert(offset <= size());
  assert(count == npos || offset + count <= size());

  return count != npos ? BufferRef(data() + offset, count)
                       : BufferRef(data() + offset, size() - offset);
}

template <>
inline BufferRef BufferBase<Buffer>::ref(size_t offset) const {
  assert(offset <= size());

  return BufferRef(data() + offset, size() - offset);
}

template <>
inline BufferRef BufferBase<Buffer>::ref(size_t offset, size_t count) const {
  assert(offset <= size());
  assert(count == npos || offset + count <= size());

  return count != npos ? BufferRef(data() + offset, count)
                       : BufferRef(data() + offset, size() - offset);
}

template <typename T>
inline BufferRef BufferBase<T>::chomp() const {
  return ends('\n') ? ref(0, size_ - 1) : ref(0, size_);
}

template <typename T>
inline BufferRef BufferBase<T>::trim() const {
  std::size_t left = 0;
  while (left < size() && std::isspace(at(left))) ++left;

  std::size_t right = size() - 1;
  while (right > 0 && std::isspace(at(right))) --right;

  return ref(left, 1 + right - left);
}

template <typename T>
Buffer BufferBase<T>::replaceAll(char src, char dst) const {
  Buffer out;
  size_t lpos = 0;

  out.reserve(size());

  for (;;) {
    size_t rpos = find(src, lpos);
    if (rpos != npos) {
      out.push_back(ref(lpos, rpos - lpos));
      out.push_back(dst);
      lpos = rpos + 1;
    } else {
      break;
    }
  }

  out.push_back(ref(lpos));

  return out;
}

template <typename T>
Buffer BufferBase<T>::replaceAll(const char* src, const char* dst) const {
  Buffer out;
  size_t lpos = 0;
  size_t slen = std::strlen(src);

  for (;;) {
    size_t rpos = find(src, lpos);
    if (rpos != npos) {
      out.push_back(ref(lpos, rpos - lpos));
      out.push_back(dst);
      lpos = rpos + slen;
    } else {
      break;
    }
  }

  out.push_back(ref(lpos));

  return out;
}

template <typename T>
inline std::string BufferBase<T>::str() const {
  return substr(0);
}

template <typename T>
inline std::string BufferBase<T>::substr(size_t offset) const {
  assert(offset <= size());
  ssize_t count = size() - offset;
  return count ? std::string(data() + offset, count) : std::string();
}

template <typename T>
inline std::string BufferBase<T>::substr(size_t offset, size_t count) const {
  assert(offset + count <= size());
  return count ? std::string(data() + offset, count) : std::string();
}

template <typename T>
template <typename U>
inline U BufferBase<T>::hex() const {
  auto i = begin();
  auto e = end();

  // empty string
  if (i == e) return 0;

  U val = 0;
  while (i != e) {
    if (!std::isxdigit(*i)) break;

    if (val) val *= 16;

    if (std::isdigit(*i))
      val += *i++ - '0';
    else if (*i >= 'a' && *i <= 'f')
      val += 10 + *i++ - 'a';
    else  // 'A' .. 'F'
      val += 10 + *i++ - 'A';
  }

  return val;
}

template <typename T>
inline bool BufferBase<T>::toBool() const {
  if (iequals(*this, "true")) return true;

  if (equals(*this, "1")) return true;

  return false;
}

template <typename T>
inline int BufferBase<T>::toInt() const {
  auto i = cbegin();
  auto e = cend();

  // empty string
  if (i == e) return 0;

  // parse sign
  bool sign = false;
  if (*i == '-') {
    sign = true;
    ++i;

    if (i == e) {
      return 0;
    }
  } else if (*i == '+') {
    ++i;

    if (i == e) return 0;
  }

  // parse digits
  int val = 0;
  while (i != e) {
    if (*i < '0' || *i > '9') break;

    if (val) val *= 10;

    val += *i++ - '0';
  }

  // parsing succeed.
  if (sign) val = -val;

  return val;
}

template <typename T>
inline double BufferBase<T>::toDouble() const {
  char* endptr = nullptr;
  double result = strtod(cbegin(), &endptr);
  if (endptr <= cend()) return result;

  return 0.0;
}

template <typename T>
inline float BufferBase<T>::toFloat() const {
  char* tmp = (char*)alloca(size() + 1);
  std::memcpy(tmp, cbegin(), size());
  tmp[size()] = '\0';

  return strtof(tmp, nullptr);
}

template <typename T>
inline std::string BufferBase<T>::hexdump(HexDumpMode mode) const {
  return BufferRef::hexdump(data(), size(), mode);
}

// --------------------------------------------------------------------------
template <typename T>
inline bool equals(const BufferBase<T>& a, const BufferBase<T>& b) {
  if (&a == &b) return true;

  if (a.size() != b.size()) return false;

  return std::memcmp(a.data(), b.data(), a.size()) == 0;
}

template <typename T, typename PodType, std::size_t N>
bool equals(const BufferBase<T>& a, PodType (&b)[N]) {
  const std::size_t bsize = N - 1;

  if (a.size() != bsize) return false;

  return std::memcmp(a.data(), b, bsize) == 0;
}

template <typename T>
inline bool equals(const BufferBase<T>& a, const std::string& b) {
  if (a.size() != b.size()) return false;

  return std::memcmp(a.data(), b.data(), b.size()) == 0;
}

inline bool equals(const std::string& a, const std::string& b) {
  if (a.size() != b.size()) return false;

  return std::memcmp(a.data(), b.data(), b.size()) == 0;
}

// --------------------------------------------------------------------

template <typename T>
inline bool iequals(const BufferBase<T>& a, const BufferBase<T>& b) {
  if (&a == &b) return true;

  if (a.size() != b.size()) return false;

  return strncasecmp(a.data(), b.data(), a.size()) == 0;
}

template <typename T, typename PodType, std::size_t N>
bool iequals(const BufferBase<T>& a, PodType (&b)[N]) {
  const std::size_t bsize = N - 1;

  if (a.size() != bsize) return false;

  return strncasecmp(a.data(), b, bsize) == 0;
}

template <typename T>
inline bool iequals(const BufferBase<T>& a, const std::string& b) {
  if (a.size() != b.size()) return false;

  return strncasecmp(a.data(), b.data(), b.size()) == 0;
}

inline bool iequals(const std::string& a, const std::string& b) {
  if (a.size() != b.size()) return false;

  return strncasecmp(a.data(), b.data(), b.size()) == 0;
}

// ------------------------------------------------------------------------
template <typename T>
inline bool operator==(const BufferBase<T>& a, const BufferBase<T>& b) {
  return equals(a, b);
}

template <typename T>
inline bool operator==(const BufferBase<T>& a, const std::string& b) {
  return equals(a, b);
}

template <typename T, typename PodType, std::size_t N>
inline bool operator==(const BufferBase<T>& a, PodType (&b)[N]) {
  return equals<T, PodType, N>(a, b);
}

template <typename T, typename PodType, std::size_t N>
inline bool operator==(PodType (&a)[N], const BufferBase<T>& b) {
  return equals<T, PodType, N>(b, a);
}

inline void immutableEnsure(void* self, size_t size) {
  MutableBuffer<immutableEnsure>* buffer =
      (MutableBuffer<immutableEnsure>*)self;
  if (size > buffer->capacity()) {
    throw std::bad_alloc();
  }
}

inline void mutableEnsure(void* self, size_t size) {
  Buffer* buffer = (Buffer*)self;
  if (size > buffer->capacity() || size == 0) {
    buffer->setCapacity(size);
  }
}
// }}}
// {{{ BufferRef impl
inline BufferRef& BufferRef::operator=(const BufferRef& v) {
  data_ = v.data_;
  size_ = v.size_;
  return *this;
}

inline void BufferRef::swap(cortex::BufferRef& other) {
  std::swap(data_, other.data_);
  std::swap(size_, other.size_);
}

inline constexpr BufferRef::const_reference_type BufferRef::operator[](
    size_t index) const {
  //assert(index < size_);

  return data_[index];
}

inline void BufferRef::shl(ssize_t value) {
  data_ -= value;
  size_ += value;
}

/** shifts view's right margin by given bytes to the right, thus, increasing
 * view's size.
 */
inline void BufferRef::shr(ssize_t value) {
  size_ += value;
}
// }}}
// {{{ MutableBuffer<ensure> impl
template <void (*ensure)(void*, size_t)>
inline MutableBuffer<ensure>::MutableBuffer()
    : BufferRef(), capacity_(0) {}

template <void (*ensure)(void*, size_t)>
inline MutableBuffer<ensure>::MutableBuffer(const MutableBuffer& v)
    : BufferRef(v), capacity_(v.capacity_) {}

template <void (*ensure)(void*, size_t)>
inline MutableBuffer<ensure>::MutableBuffer(MutableBuffer&& v)
    : BufferRef(std::move(v)), capacity_(std::move(v.capacity_)) {
  v.data_ = nullptr;
  v.size_ = 0;
  v.capacity_ = 0;
}

template <void (*ensure)(void*, size_t)>
inline MutableBuffer<ensure>::MutableBuffer(char* value, size_t capacity,
                                            size_t size)
    : BufferRef(value, size), capacity_(capacity) {}

template <void (*ensure)(void*, size_t)>
inline bool MutableBuffer<ensure>::resize(size_t value) {
  if (value > capacity_) return false;

  size_ = value;
  return true;
}

template <void (*ensure)(void*, size_t)>
inline size_t MutableBuffer<ensure>::capacity() const {
  return capacity_;
}

template <void (*ensure)(void*, size_t)>
inline void MutableBuffer<ensure>::reserve(size_t value) {
  ensure(this, value);
}

// TODO: implement operator bool() and verify it's working for:
//     if (BufferRef r = foo()) {}
//     if (foo()) {}
//     if (someBufferRef) {}

template <void (*ensure)(void*, size_t)>
inline bool MutableBuffer<ensure>::operator!() const {
  return empty();
}

template <void (*ensure)(void*, size_t)>
inline void MutableBuffer<ensure>::push_back(value_type value) {
  reserve(size() + sizeof(value));
  data_[size_++] = value;
}

template <void (*ensure)(void*, size_t)>
void MutableBuffer<ensure>::push_back(value_type value, size_t count) {
  reserve(size() + sizeof(value) * count);

  memset(data_ + size_, value, count);
  size_ += count;
}

template <void (*ensure)(void*, size_t)>
inline void MutableBuffer<ensure>::push_back(int value) {
  char buf[32];
  int n = std::snprintf(buf, sizeof(buf), "%d", value);
  push_back(buf, n);
}

template <void (*ensure)(void*, size_t)>
inline void MutableBuffer<ensure>::push_back(long value) {
  char buf[32];
  int n = std::snprintf(buf, sizeof(buf), "%ld", value);
  push_back(buf, n);
}

template <void (*ensure)(void*, size_t)>
inline void MutableBuffer<ensure>::push_back(long long value) {
  char buf[32];
  int n = std::snprintf(buf, sizeof(buf), "%lld", value);
  push_back(buf, n);
}

template <void (*ensure)(void*, size_t)>
inline void MutableBuffer<ensure>::push_back(unsigned value) {
  char buf[32];
  int n = std::snprintf(buf, sizeof(buf), "%u", value);
  push_back(buf, n);
}

template <void (*ensure)(void*, size_t)>
inline void MutableBuffer<ensure>::push_back(unsigned long value) {
  char buf[32];
  int n = std::snprintf(buf, sizeof(buf), "%lu", value);
  push_back(buf, n);
}

template <void (*ensure)(void*, size_t)>
inline void MutableBuffer<ensure>::push_back(unsigned long long value) {
  char buf[32];
  int n = std::snprintf(buf, sizeof(buf), "%llu", value);
  push_back(buf, n);
}

template <void (*ensure)(void*, size_t)>
inline void MutableBuffer<ensure>::push_back(const BufferRef& value) {
  if (size_t len = value.size()) {
    reserve(size_ + len);
    std::memcpy(end(), value.cbegin(), len);
    size_ += len;
  }
}

template <void (*ensure)(void*, size_t)>
inline void MutableBuffer<ensure>::push_back(const BufferRef& value,
                                             size_t offset, size_t length) {
  assert(value.size() <= offset + length);

  if (!length)
    return;

  reserve(size_ + length);

  memcpy(end(), value.cbegin() + offset, length);
  size_ += length;
}

template <void (*ensure)(void*, size_t)>
inline void MutableBuffer<ensure>::push_back(const std::string& value) {
  if (size_t len = value.size()) {
    reserve(size_ + len);
    std::memcpy(end(), value.data(), len);
    size_ += len;
  }
}

template <void (*ensure)(void*, size_t)>
template <typename PodType, size_t N>
inline void MutableBuffer<ensure>::push_back(PodType (&value)[N]) {
  push_back(reinterpret_cast<const void*>(value), N - 1);
}

template <void (*ensure)(void*, size_t)>
inline void MutableBuffer<ensure>::push_back(const void* value, size_t size) {
  if (size) {
    reserve(size_ + size);
    const size_t minsize = std::min(capacity_ - size_, size);
    std::memcpy(end(), value, minsize);
    size_ += minsize;
  }
}

template <void (*ensure)(void*, size_t)>
inline size_t MutableBuffer<ensure>::vprintf(const char* fmt, va_list args) {
  size_t initialLength = size();
  reserve(size() + strlen(fmt) + 1);

  while (true) {
    va_list va;
    va_copy(va, args);
    ssize_t buflen = vsnprintf(data_ + size_, capacity_ - size_, fmt, va);
    va_end(va);

    if (buflen >= -1 && buflen < static_cast<ssize_t>(capacity_ - size_)) {
      resize(size_ + buflen);
      break;  // success
    }

    buflen = buflen > -1 ? buflen + 1      // glibc >= 2.1
                         : capacity_ * 2;  // glibc <= 2.0

    reserve(capacity_ + buflen);
  }

  return size() - initialLength;
}

template <void (*ensure)(void*, size_t)>
inline size_t MutableBuffer<ensure>::printf(const char* fmt) {
  const size_t initialLength = size();
  push_back(fmt);
  return size() - initialLength;
}

template <void (*ensure)(void*, size_t)>
template <typename... Args>
inline size_t MutableBuffer<ensure>::printf(const char* fmt, Args... args) {
  const size_t initialLength = size();
  reserve(size() + strlen(fmt) + 1);

  while (true) {
    ssize_t buflen = snprintf(data_ + size_, capacity_ - size_, fmt, args...);

    if (buflen >= -1 && buflen < static_cast<ssize_t>(capacity_ - size_)) {
      resize(size_ + buflen);
      break;  // success
    }

    buflen = buflen > -1 ? buflen + 1      // glibc >= 2.1
                         : capacity_ * 2;  // glibc <= 2.0

    reserve(capacity_ + buflen);
  }

  return size() - initialLength;
}

template <void (*ensure)(void*, size_t)>
inline typename MutableBuffer<ensure>::reference_type MutableBuffer<ensure>::
operator[](size_t index) {
  assert(index < size_);

  return data_[index];
}

template <void (*ensure)(void*, size_t)>
inline typename MutableBuffer<ensure>::const_reference_type
MutableBuffer<ensure>::
operator[](size_t index) const {
  assert(index < size_);

  return data_[index];
}

template <void (*ensure)(void*, size_t)>
inline const Buffer::value_type* MutableBuffer<ensure>::c_str() const {
  const_cast<MutableBuffer<ensure>*>(this)->reserve(size_ + 1);
  const_cast<MutableBuffer<ensure>*>(this)->data_[size_] = '\0';

  return data_;
}
// }}}
// {{{ free functions impl
inline Buffer& operator<<(Buffer& b, Buffer::value_type v) {
  b.push_back(v);
  return b;
}

inline Buffer& operator<<(Buffer& b, int v) {
  b.push_back(v);
  return b;
}

inline Buffer& operator<<(Buffer& b, long v) {
  b.push_back(v);
  return b;
}

inline Buffer& operator<<(Buffer& b, long long v) {
  b.push_back(v);
  return b;
}

inline Buffer& operator<<(Buffer& b, unsigned v) {
  b.push_back(v);
  return b;
}

inline Buffer& operator<<(Buffer& b, unsigned long v) {
  b.push_back(v);
  return b;
}

inline Buffer& operator<<(Buffer& b, unsigned long long v) {
  b.push_back(v);
  return b;
}

inline Buffer& operator<<(Buffer& b, const Buffer& v) {
  b.push_back(v);
  return b;
}

inline Buffer& operator<<(Buffer& b, const BufferRef& v) {
  b.push_back(v);
  return b;
}

inline Buffer& operator<<(Buffer& b, const std::string& v) {
  b.push_back(v);
  return b;
}

template <typename PodType, size_t N>
inline Buffer& operator<<(Buffer& b, PodType (&v)[N]) {
  b.template push_back<PodType, N>(v);
  return b;
}

inline Buffer& operator<<(Buffer& b, typename Buffer::value_type* v) {
  b.push_back(v);
  return b;
}

inline Buffer& operator+=(Buffer& b, const BufferRef& v) {
  b.push_back(v);
  return b;
}

inline Buffer& operator+=(Buffer& b, const std::string& v) {
  b.push_back(v);
  return b;
}

inline Buffer& operator+=(Buffer& b, typename Buffer::value_type v) {
  b.push_back(v);
  return b;
}

template <typename PodType, size_t N>
inline Buffer& operator+=(Buffer& b, PodType (&v)[N]) {
  b.template push_back<PodType, N>(v);
  return b;
}
// }}}
// {{{ FixedBuffer impl
inline FixedBuffer::FixedBuffer() : MutableBuffer<immutableEnsure>() {}

inline FixedBuffer::FixedBuffer(const FixedBuffer& v)
    : MutableBuffer<immutableEnsure>(v) {}

inline FixedBuffer::FixedBuffer(FixedBuffer&& v)
    : MutableBuffer<immutableEnsure>(v.data(), v.capacity(), v.size()) {
  v.data_ = nullptr;
  v.capacity_ = 0;
  v.size_ = 0;
}

inline FixedBuffer::FixedBuffer(char* data, size_t capacity, size_t size)
    : MutableBuffer<immutableEnsure>(data, capacity, size) {
}

inline Buffer& Buffer::operator=(Buffer&& v) {
  swap(v);
  v.clear();
  v.setMark(0);

  return *this;
}

inline FixedBuffer& FixedBuffer::operator=(const FixedBuffer& v) {
  clear();
  push_back(v.data(), v.size());
  return *this;
}

inline FixedBuffer& FixedBuffer::operator=(const Buffer& v) {
  clear();
  push_back(v.data(), v.size());
  return *this;
}

inline FixedBuffer& FixedBuffer::operator=(const BufferRef& v) {
  clear();
  push_back(v);
  return *this;
}

inline FixedBuffer& FixedBuffer::operator=(const std::string& v) {
  clear();
  push_back(v);
  return *this;
}

inline FixedBuffer& FixedBuffer::operator=(const value_type* v) {
  clear();
  push_back(v);
  return *this;
}
// }}}
// {{{ Buffer impl
inline Buffer::Buffer()
    : MutableBuffer<mutableEnsure>(),
      mark_(0) {
}

inline Buffer::Buffer(const BufferRef& v, size_t offset, size_t count)
    : MutableBuffer<mutableEnsure>(),
      mark_(0) {
  assert(offset + count <= v.size());

  push_back(v.data() + offset, count);
}

inline Buffer::Buffer(const BufferRef& v)
    : MutableBuffer<mutableEnsure>(),
      mark_(0) {
  push_back(v.data(), v.size());
}

inline Buffer::Buffer(size_t _capacity)
    : MutableBuffer<mutableEnsure>(),
      mark_(0) {
  reserve(_capacity);
}

template <typename PodType, size_t N>
inline Buffer::Buffer(PodType (&value)[N])
    : MutableBuffer<mutableEnsure>(),
      mark_(0) {
  reserve(N);
  push_back(value, N - 1);
}

inline Buffer::Buffer(const value_type* value, size_t size)
    : MutableBuffer<mutableEnsure>(),
      mark_(0) {
  reserve(size + 1);
  push_back(value, size);
}

inline Buffer::Buffer(const char* v)
    : MutableBuffer<mutableEnsure>(),
      mark_(0) {
  push_back(v);
}

inline Buffer::Buffer(const std::string& v)
    : MutableBuffer<mutableEnsure>(),
      mark_(0) {
  reserve(v.size() + 1);
  push_back(v.data(), v.size());
}

inline Buffer::Buffer(const Buffer& v)
    : MutableBuffer<mutableEnsure>(),
      mark_(0) {
  push_back(v.data(), v.size());
}

inline Buffer::Buffer(Buffer&& v)
    : MutableBuffer<mutableEnsure>(std::move(v)),
      mark_(0) {
}

inline Buffer::~Buffer() {
  reserve(0);
}

inline Buffer& Buffer::operator=(const Buffer& v) {
  clear();
  push_back(v);

  return *this;
}

inline Buffer& Buffer::operator=(const BufferRef& v) {
  clear();
  push_back(v);

  return *this;
}

inline Buffer& Buffer::operator=(const std::string& v) {
  clear();
  push_back(v.data(), v.size());

  return *this;
}

inline Buffer& Buffer::operator=(const value_type* v) {
  clear();
  push_back(v, std::strlen(v));

  return *this;
}

inline void Buffer::swap(cortex::Buffer& other) {
  std::swap(data_, other.data_);
  std::swap(size_, other.size_);
  std::swap(capacity_, other.capacity_);
  std::swap(mark_, other.mark_);
}

inline size_t Buffer::mark() const CORTEX_NOEXCEPT {
  return mark_;
}

inline void Buffer::setMark(size_t value) {
  mark_ = value;
}

inline BufferSlice Buffer::slice(size_t offset) const {
  assert(offset <= size());
  return BufferSlice(*(Buffer*)this, offset, size() - offset);
}

inline BufferSlice Buffer::slice(size_t offset, size_t count) const {
  assert(offset <= size());
  assert(count == npos || offset + count <= size());

  return count != npos ? BufferSlice(*(Buffer*)this, offset, count)
                       : BufferSlice(*(Buffer*)this, offset, size() - offset);
}

inline Buffer Buffer::fromCopy(const value_type* data, size_t count) {
  Buffer result(count);
  result.push_back(data, count);
  return result;
}
// }}}
// {{{ BufferSlice impl
inline BufferSlice::BufferSlice() : BufferBase<Buffer>() {
}

inline BufferSlice::BufferSlice(Buffer& buffer, size_t offset, size_t size)
    : BufferBase<Buffer>(data_type(&buffer, offset), size) {
}

inline BufferSlice::BufferSlice(const BufferSlice& v)
    : BufferBase<Buffer>(v) {
}

inline BufferSlice& BufferSlice::operator=(const BufferSlice& v) {
  data_ = v.data_;
  size_ = v.size_;

  return *this;
}

inline BufferSlice BufferSlice::slice(size_t offset) const {
  assert(offset <= size());
  return BufferSlice(*data_.buffer(), data_.offset() + offset, size() - offset);
}

inline BufferSlice BufferSlice::slice(size_t offset, size_t count) const {
  assert(offset <= size());
  assert(count == npos || offset + count <= size());

  return count != npos
             ? BufferSlice(*data_.buffer(), data_.offset() + offset, count)
             : BufferSlice(*data_.buffer(), data_.offset() + offset,
                           size() - offset);
}

/** Shifts view's left margin by given bytes to the left, thus, increasing
 *  view's size.
 */
inline void BufferSlice::shl(ssize_t value) {
  assert(data_.offset_ - value >= 0);
  data_.offset_ -= value;
}

/** Shifts view's right margin by given bytes to the right, thus, increasing
 *  view's size.
 */
inline void BufferSlice::shr(ssize_t value) { size_ += value; }
// }}}
// {{{ free functions (concatenation) impl
inline Buffer operator+(const BufferRef& a, const BufferRef& b) {
  Buffer buf(a.size() + b.size() + 1);
  buf.push_back(a);
  buf.push_back(b);
  return buf;
}

inline void swap(cortex::Buffer& left, cortex::Buffer& right) {
  left.swap(right);
}

inline void swap(cortex::BufferRef& left, cortex::BufferRef& right) {
  left.swap(right);
}
// }}}
}  // namespace cortex

// {{{ std::hash<BufferBase<T>>
namespace cortex {
  // Fowler / Noll / Vo (FNV) Hash-Implementation
  template <typename T>
  uint32_t _hash(const T& array) CORTEX_NOEXCEPT {
    uint32_t result = 2166136261u;

    for (auto value : array) {
      result ^= value;
      result *= 16777619;
    }

    return result;
  }
}

namespace std {
template <>
struct hash<cortex::BufferSlice> {
  typedef cortex::BufferSlice argument_type;
  typedef uint32_t result_type;

  result_type operator()(const argument_type& value) const CORTEX_NOEXCEPT {
    return cortex::_hash(value);
  }
};

template <>
struct hash<cortex::BufferRef> {
  typedef cortex::BufferRef argument_type;
  typedef uint32_t result_type;

  result_type operator()(const argument_type& value) const CORTEX_NOEXCEPT {
    return cortex::_hash(value);
  }
};

template <>
struct hash<cortex::Buffer> {
  typedef cortex::Buffer argument_type;
  typedef uint32_t result_type;

  result_type operator()(const argument_type& value) const CORTEX_NOEXCEPT {
    return cortex::_hash(value);
  }
};

inline ostream& operator<<(ostream& os, const cortex::BufferRef& b) {
  os << b.str();
  return os;
}

inline ostream& operator<<(ostream& os, const cortex::Buffer& b) {
  os << b.str();
  return os;
}
}
// }}}
