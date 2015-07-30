/**
 * This file is part of the "libsstable" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/buffer.h>
#include <stx/io/inputstream.h>
#include <stx/io/outputstream.h>

namespace stx {
namespace sstable {

class FileHeader {
  friend class FileHeaderReader;
public:

  /**
   * Read and verify the header from the provided input stream
   */
  static bool verifyHeader(InputStream* is);

  /**
   * Read the header meta page, but not the userdata from the provided input
   * stream
   */
  static FileHeader readMetaPage(InputStream* is);

  /**
   * Read the full header (meta page + userdata) from the provided input stream
   */
  static FileHeader readHeader(
      Buffer* userdata,
      InputStream* is);

  /**
   * Write the header meta page, but not the userdata to the provided output
   * stream
   */
  static void writeMetaPage(
      const FileHeader& header,
      OutputStream* os);

  /**
   * Write the full header (meta page + userdata) to the provided output stream
   */
  static void writeHeader(
      const FileHeader& header,
      const void* userdata,
      size_t userdata_size,
      OutputStream* os);

  /**
   * Create a new file header from the provided arguments
   */
  FileHeader(
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
   * Returns the body size in bytes
   */
  size_t bodySize() const;

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
  FileHeader();

  uint16_t version_;
  uint64_t flags_;
  uint64_t body_size_;
  uint32_t userdata_checksum_;
  uint32_t userdata_size_;
};

}
}
