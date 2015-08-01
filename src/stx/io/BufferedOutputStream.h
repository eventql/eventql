/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include "stx/io/outputstream.h"

namespace stx {

/**
 * A buffered output stream allowsan application to write bytes to the
 * underlying output stream without necessarily causing a call to the
 * underlying system for each byte written.
*/
class BufferedOutputStream : public OutputStream {
public:
  static const constexpr size_t kDefaultBufferSize = 4096;

  /**
   * Create a new BufferedOutputStream instance from the provided output stream
   *
   * @param stream the stream
   * @param buffer_size the spool buffer size
   */
  static ScopedPtr<BufferedOutputStream> fromStream(
      ScopedPtr<OutputStream> stream,
      size_t buffer_size = kDefaultBufferSize);

  /**
   * Create a new BufferedOutputStream instance from the provided output stream
   *
   * @param stream the stream
   * @param buffer_size the spool buffer size
   */
  BufferedOutputStream(
      ScopedPtr<OutputStream> stream,
      size_t buffer_size = kDefaultBufferSize);

  /**
   * Flushes all remaining contents on destrory
   */
  ~BufferedOutputStream();

  BufferedOutputStream(const BufferedOutputStream& other) = delete;
  BufferedOutputStream& operator=(const BufferedOutputStream& other) = delete;

  /**
   * Write the next n bytes to the stream. This may raise an exception.
   * Returns the number of bytes that have been written.
   *
   * @param data a pointer to the data to be written
   * @param size then number of bytes to be written
   */
  size_t write(const char* data, size_t size) override;

  /**
   * Flushes the buffer to the underlying output stream
   */
  void flush();

protected:
  ScopedPtr<OutputStream> os_;
  Buffer buf_;
};

}
