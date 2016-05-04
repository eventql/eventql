/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/io/file.h>
#include <eventql/util/io/pagemanager.h>
#include <eventql/infra/sstable/MetaPage.h>
#include <eventql/util/exception.h>

namespace stx {
namespace sstable {

class SSTableColumnWriter;
class SSTableColumnSchema;

class SSTableWriter {
public:

  /**
   * Create and open a new sstable for writing
   */
  static std::unique_ptr<SSTableWriter> create(
      const std::string& filename,
      void const* header,
      size_t header_size);

  /**
   * Re-open a partially written sstable for writing
   */
  static std::unique_ptr<SSTableWriter> reopen(
      const std::string& filename);

  SSTableWriter(const SSTableWriter& other) = delete;
  SSTableWriter& operator=(const SSTableWriter& other) = delete;
  ~SSTableWriter();

  /**
   * Append a row to the sstable
   */
  uint64_t appendRow(
      void const* key,
      size_t key_size,
      void const* data,
      size_t data_size);

  /**
   * Append a row to the sstable
   */
  uint64_t appendRow(
      const std::string& key,
      const std::string& value);

  /**
   * Append a row to the sstable
   */
  uint64_t appendRow(
      const std::string& key,
      const SSTableColumnWriter& columns);

  /**
   * Commit written rows // metadata to disk
   */
  void commit();

  void writeFooter(uint32_t footer_type, void* data, size_t size);
  void writeFooter(uint32_t footer_type, const Buffer& buf);

protected:

  SSTableWriter(
      File&& file,
      MetaPage hdr);

private:
  File file_;
  MetaPage hdr_;
  bool meta_dirty_;
};


}
}

