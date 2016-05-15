/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#pragma once
#include "eventql/util/io/outputstream.h"

namespace stx {

/**
 * A buffered output stream allows an application to write bytes to the
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
