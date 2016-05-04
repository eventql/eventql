/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORDMETRIC_METRICDB_FILEHEADERWRITER_H
#define _FNORDMETRIC_METRICDB_FILEHEADERWRITER_H
#include <stx/util/binarymessagewriter.h>
#include <stx/buffer.h>
#include <stx/io/outputstream.h>
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <eventql/infra/sstable/MetaPage.h>

namespace stx {
namespace sstable {

class FileHeaderWriter : public stx::util::BinaryMessageWriter {
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
}

#endif
