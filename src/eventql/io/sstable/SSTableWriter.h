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
#include <eventql/util/stdtypes.h>
#include <eventql/util/io/file.h>
#include <eventql/util/io/pagemanager.h>
#include <eventql/io/sstable/MetaPage.h>
#include <eventql/util/exception.h>


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

