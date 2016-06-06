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
#include <eventql/util/io/file.h>
#include <eventql/io/cstable/ColumnReader.h>
#include <eventql/io/cstable/cstable.h>
#include <eventql/io/cstable/page_manager.h>

namespace cstable {

class CSTableFile {
public:

  CSTableFile(
      BinaryFormatVersion version,
      const TableSchema& schema,
      int fd = -1);

  BinaryFormatVersion getBinaryFormatVersion() const;
  const TableSchema& getTableSchema() const;
  const PageManager* getPageManager() const;
  PageManager* getPageManager();

  void commitTransaction(uint64_t transaction_id, uint64_t num_rows);
  void getTransaction(uint64_t* transaction_id, uint64_t* num_rows) const;

  void writeFile(int fd);
  void writeFileHeader(int fd);
  void writeFilePages(int fd);
  void writeFileIndex(int fd, uint64_t* index_offset, uint64_t* index_size);
  void writeFileTransaction(
      int fd,
      uint64_t index_offset,
      uint64_t index_size);

protected:
  BinaryFormatVersion version_;
  TableSchema schema_;
  Buffer file_header_;
  mutable std::mutex mutex_;
  uint64_t transaction_id_;
  uint64_t num_rows_;
  ScopedPtr<PageManager> page_mgr_;
};

} // namespace cstable


