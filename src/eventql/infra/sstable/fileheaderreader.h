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
#ifndef _FNORDMETRIC_METRICDB_FILEHEADERREADER_H
#define _FNORDMETRIC_METRICDB_FILEHEADERREADER_H
#include <eventql/util/util/binarymessagereader.h>
#include <eventql/util/io/inputstream.h>
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <eventql/infra/sstable/MetaPage.h>

namespace sstable {

class FileHeaderReader : public util::BinaryMessageReader {
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

#endif
