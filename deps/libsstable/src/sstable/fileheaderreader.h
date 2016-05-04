/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORDMETRIC_METRICDB_FILEHEADERREADER_H
#define _FNORDMETRIC_METRICDB_FILEHEADERREADER_H
#include <stx/util/binarymessagereader.h>
#include <stx/io/inputstream.h>
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <sstable/MetaPage.h>

namespace stx {
namespace sstable {

class FileHeaderReader : public stx::util::BinaryMessageReader {
public:

  /**
   * Read and verify the header from the provided input stream
   */
  static bool verifyHeader(InputStream* is);

  /**
   * Read the header meta page, but not the userdata from the provided input
   * stream
   */
  static MetaPage readMetaPage(InputStream* is);

  /**
   * Read the full header (meta page + userdata) from the provided input stream
   */
  static MetaPage readHeader(
      Buffer* userdata,
      InputStream* is);


  /**
   * DEPRECATED
   */
  FileHeaderReader(
      void* buf,
      size_t buf_size);

  /**
   * DEPRECATED Verify the checksums and boundaries. Returns true if valid, false otherwise
   */
  bool verify();

  /**
   * DEPRECATED Returns the size of the header, including userdata in bytes
   */
  size_t headerSize() const;

  /**
   * DEPRECATED Returns true iff the table is finalized (immutable)
   */
  bool isFinalized() const;

  /**
   * DEPRECATED Returns the body size in bytes
   */
  size_t bodySize() const;

  /**
   * DEPRECATED Returns the header userdata size in bytes
   */
  size_t userdataSize() const;

  /**
   * DEPRECATED Return the userdata
   */
  void readUserdata(const void** userdata, size_t* userdata_size);

protected:
  MetaPage hdr_;
  size_t file_size_;
};

}
}

#endif
