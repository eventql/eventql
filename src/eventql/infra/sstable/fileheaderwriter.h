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
#ifndef _FNORDMETRIC_METRICDB_FILEHEADERWRITER_H
#define _FNORDMETRIC_METRICDB_FILEHEADERWRITER_H
#include <eventql/util/util/binarymessagewriter.h>
#include <eventql/util/buffer.h>
#include <eventql/util/io/outputstream.h>
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <eventql/infra/sstable/MetaPage.h>


namespace sstable {

class FileHeaderWriter : public util::BinaryMessageWriter {
public:

  /**
   * Write the header meta page, but not the userdata to the provided output
   * stream
   */
  static void writeMetaPage(
      const MetaPage& header,
      OutputStream* os);

  /**
   * Write the full header (meta page + userdata) to the provided output stream
   */
  static void writeHeader(
      const MetaPage& header,
      const void* userdata,
      size_t userdata_size,
      OutputStream* os);

  /**
   * DEPRECATED
   */
  static size_t calculateSize(size_t userdata_size);

  /**
   * DEPRECATED
   * Write a new file header
   */
  FileHeaderWriter(
      void* buf,
      size_t buf_size,
      size_t body_size,
      const void* userdate,
      size_t userdata_size);

  /**
   * DEPRECATED
   */
  FileHeaderWriter(
      void* buf,
      size_t buf_size);

  /**
   * DEPRECATED
   */
  void updateBodySize(size_t body_size);

  /**
   * DEPRECATED
   */
  void setFlag(FileHeaderFlags flag);

};

}

#endif
