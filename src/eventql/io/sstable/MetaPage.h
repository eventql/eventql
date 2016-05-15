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
#include <eventql/util/stdtypes.h>
#include <eventql/util/buffer.h>
#include <eventql/util/io/inputstream.h>
#include <eventql/util/io/outputstream.h>

namespace sstable {

class MetaPage {
  friend class FileHeaderReader;
public:

  /**
   * Create a new file header from the provided arguments
   */
  MetaPage(
      const void* userdata,
      size_t userdata_size);

  /**
   * Returns the file version
   */
  uint16_t version() const;

  /**
   * Returns the size of the header, including userdata in bytes
   */
  size_t headerSize() const;

  /**
   * Returns true iff the table is finalized (immutable)
   */
  bool isFinalized() const;

  /**
   * Returns the number of rows in this table
   */
  size_t rowCount() const;

  /**
   * Set the number of rows in this table
   */
  void setRowCount(size_t new_row_count);

  /**
   * Returns the body size in bytes
   */
  size_t bodySize() const;

  /**
   * Set the body size
   */
  void setBodySize(size_t new_body_size);

  /**
   * Returns the body offset in bytes
   */
  size_t bodyOffset() const;

  /**
   * Returns the header userdata size in bytes
   */
  size_t userdataSize() const;

  /**
   * Returns the file offset of the header userdata in bytes
   */
  size_t userdataOffset() const;

  /**
   * Returns the userdata checksum
   */
  uint32_t userdataChecksum() const;

  /**
   * Returns the raw flags
   */
  uint64_t flags() const;

protected:
  MetaPage();

  uint16_t version_;
  uint64_t flags_;
  uint64_t body_size_;
  uint64_t num_rows_;
  uint32_t userdata_checksum_;
  uint32_t userdata_size_;
};

}
