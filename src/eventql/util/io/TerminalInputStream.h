/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include "eventql/util/io/inputstream.h"

class TerminalInputStream : public InputStream {
public:

  /**
   * Create a new TerminalOutputStream instance from the provided output stream
   *
   * @param stream the stream
   */
  static ScopedPtr<TerminalInputStream> fromStream(
      ScopedPtr<FileInputStream> stream);

  /**
   * Create a new TerminalOutputStream instance from the provided output stream
   *
   * @param stream the stream
   */
  TerminalInputStream(
      ScopedPtr<FileInputStream> stream);

  /**
   * Read the next byte from the input stream. Returns true if the next byte was
   * read and false if the end of the stream was reached.
   *
   * @param target the target char pointer
   */
  bool readNextByte(char* target) override;

  /**
   * Read N bytes from the stream and copy the data into the provided buffer
   * Returns the number of bytes read.
   *
   * @param target the string to copy the data into
   * @param n_bytes the number of bytes to read
   */
  size_t readNextBytes(void* target, size_t n_bytes) override;

  /**
   * Skip the next N bytes in the stream. Returns the number of bytes skipped.
   *
   * @param n_bytes the number of bytes to skip
   */
  size_t skipNextBytes(size_t n_bytes) override;

  /**
   * Check if the end of this input stream was reached. Returns true if the
   * end was reached, false otherwise
   */
  bool eof() override;

protected:
  ScopedPtr<InputStream> is_;
};
