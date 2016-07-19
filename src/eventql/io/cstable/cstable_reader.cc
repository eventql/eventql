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
#include "eventql/eventql.h"
#include <eventql/io/cstable/cstable_reader.h>
#include <eventql/io/cstable/io/PageReader.h>
#include <eventql/io/cstable/columns/v1/BooleanColumnReader.h>
#include <eventql/io/cstable/columns/v1/BitPackedIntColumnReader.h>
#include <eventql/io/cstable/columns/v1/UInt32ColumnReader.h>
#include <eventql/io/cstable/columns/v1/UInt64ColumnReader.h>
#include <eventql/io/cstable/columns/v1/LEB128ColumnReader.h>
#include <eventql/io/cstable/columns/v1/DoubleColumnReader.h>
#include <eventql/io/cstable/columns/v1/StringColumnReader.h>
#include <eventql/io/cstable/columns/column_reader_uint.h>
#include <eventql/io/cstable/columns/column_reader_string.h>
#include <eventql/io/cstable/columns/column_reader_float.h>
#include <eventql/io/cstable/columns/page_reader_bitpacked.h>
#include <eventql/util/io/file.h>
#include <eventql/util/io/mmappedfile.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace cstable {

static RefPtr<v1::ColumnReader> openColumnV1(
    const ColumnConfig& c,
    RefPtr<MmappedFile> mmap) {
  auto csize = c.body_size;
  auto cdata = mmap->structAt<void>(c.body_offset);
  auto rmax = c.rlevel_max;
  auto dmax = c.dlevel_max;

  switch (c.storage_type) {
    case ColumnEncoding::BOOLEAN_BITPACKED:
      return new v1::BooleanColumnReader(rmax, dmax, cdata, csize);
    case ColumnEncoding::UINT32_BITPACKED:
      return new v1::BitPackedIntColumnReader(rmax, dmax, cdata, csize);
    case ColumnEncoding::UINT32_PLAIN:
      return new v1::UInt32ColumnReader(rmax, dmax, cdata, csize);
    case ColumnEncoding::UINT64_PLAIN:
      return new v1::UInt64ColumnReader(rmax, dmax, cdata, csize);
    case ColumnEncoding::UINT64_LEB128:
      return new v1::LEB128ColumnReader(rmax, dmax, cdata, csize);
    case ColumnEncoding::FLOAT_IEEE754:
      return new v1::DoubleColumnReader(rmax, dmax, cdata, csize);
    case ColumnEncoding::STRING_PLAIN:
      return new v1::StringColumnReader(rmax, dmax, cdata, csize);
    default:
      RAISEF(
          kRuntimeError,
          "unsupported column type: $0",
          (uint32_t) c.storage_type);
  }
}

static RefPtr<ColumnReader> openColumnV2(
    const ColumnConfig& c,
    const PageManager* page_mgr) {
  ScopedPtr<UnsignedIntPageReader> rlevel_reader;
  ScopedPtr<UnsignedIntPageReader> dlevel_reader;

  if (c.rlevel_max > 0) {
    PageIndexKey rlevel_idx_key {
      .column_id = c.column_id,
      .entry_type = PageIndexEntryType::RLEVEL
    };

    rlevel_reader = mkScoped(
        new BitPackedIntPageReader(rlevel_idx_key, page_mgr));
  }

  if (c.dlevel_max > 0) {
    PageIndexKey dlevel_idx_key {
      .column_id = c.column_id,
      .entry_type = PageIndexEntryType::DLEVEL
    };

    dlevel_reader = mkScoped(
        new BitPackedIntPageReader(dlevel_idx_key, page_mgr));
  }

  switch (c.logical_type) {
    case ColumnType::BOOLEAN:
    case ColumnType::UNSIGNED_INT:
    case ColumnType::DATETIME:
      return new UnsignedIntColumnReader(
          c,
          std::move(rlevel_reader),
          std::move(dlevel_reader),
          page_mgr);
    case ColumnType::STRING:
      return new StringColumnReader(
          c,
          std::move(rlevel_reader),
          std::move(dlevel_reader),
          page_mgr);
    case ColumnType::FLOAT:
      return new FloatColumnReader(
          c,
          std::move(rlevel_reader),
          std::move(dlevel_reader),
          page_mgr);
    default:
      RAISEF(
          kRuntimeError,
          "unsupported column type: $0",
          (uint32_t) c.storage_type);
  }
}

RefPtr<CSTableReader> CSTableReader::openFile(const String& filename) {
  auto file = File::openFile(filename, File::O_READ);
  auto file_is = FileInputStream::fromFileDescriptor(file.fd());

  BinaryFormatVersion version;
  FileHeader header;
  MetaBlock metablock;
  Option<PageRef> free_index;
  readHeader(
      &version,
      &header,
      &metablock,
      &free_index,
      file_is.get());

  switch (version) {
    case BinaryFormatVersion::v0_1_0: {
      auto mmap = mkRef(new MmappedFile(std::move(file)));

      Vector<Function<RefPtr<ColumnReader> ()>> column_reader_factories;
      for (const auto& col : header.columns) {
        auto factory = [col, mmap] () -> RefPtr<ColumnReader> {
          return openColumnV1(col, mmap.get()).get();
        };

        column_reader_factories.emplace_back(factory);
      }

      return new CSTableReader(
          version,
          nullptr,
          true,
          header.columns,
          column_reader_factories,
          header.num_rows,
          -1);
    }

    case BinaryFormatVersion::v0_2_0: {
      Vector<PageIndexEntry> index;
      {
        file_is->seekTo(metablock.index_offset);
        v0_2_0::readIndex(&index, file_is.get());
      }

      auto page_mgr = mkScoped(new PageManager(file.fd(), 0, index));
      auto page_mgr_ptr = page_mgr.get();

      Vector<Function<RefPtr<ColumnReader> ()>> column_reader_factories;
      for (const auto& col : header.columns) {
        auto factory = [col, page_mgr_ptr] () -> RefPtr<ColumnReader> {
          return openColumnV2(col, page_mgr_ptr);
        };

        column_reader_factories.emplace_back(factory);
      }

      return new CSTableReader(
          version,
          page_mgr.release(),
          true,
          header.columns,
          column_reader_factories,
          metablock.num_rows,
          file.releaseFD());
    }
  }
}

RefPtr<CSTableReader> CSTableReader::openFile(
    const CSTableFile* file,
    uint64_t limit /* = -1 */) {
  uint64_t transaction_id;
  uint64_t num_rows;
  file->getTransaction(&transaction_id, &num_rows);

  auto columns = file->getTableSchema().flatColumns();
  auto page_mgr_ptr = file->getPageManager();
  Vector<Function<RefPtr<ColumnReader> ()>> column_reader_factories;
  for (const auto& col : columns) {
    auto factory = [col, page_mgr_ptr] () -> RefPtr<ColumnReader> {
      return openColumnV2(col, page_mgr_ptr);
    };

    column_reader_factories.emplace_back(factory);
  }

  return new CSTableReader(
      file->getBinaryFormatVersion(),
      file->getPageManager(),
      false,
      columns,
      column_reader_factories,
      std::min(num_rows, limit),
      -1);
}

CSTableReader::CSTableReader(
    BinaryFormatVersion version,
    const PageManager* page_mgr,
    bool page_mgr_owned,
    Vector<ColumnConfig> columns,
    Vector<Function<RefPtr<ColumnReader> ()>> column_reader_factories,
    uint64_t num_rows,
    int fd) :
    version_(version),
    page_mgr_(page_mgr),
    page_mgr_owned_(page_mgr_owned),
    columns_(columns),
    column_reader_factories_(column_reader_factories),
    column_readers_shared_(columns.size()),
    num_rows_(num_rows),
    fd_(fd) {
  RCHECK(column_reader_factories.size() == columns.size(), "illegal column list");

  for (size_t i = 0; i < columns.size(); ++i) {
    columns_by_name_.emplace(columns_[i].column_name, i);
  }
}

CSTableReader::~CSTableReader() {
  if (fd_ > 0) {
    close(fd_);
  }

  if (page_mgr_ && page_mgr_owned_) {
    delete page_mgr_;
  }
}

RefPtr<ColumnReader> CSTableReader::getColumnReader(
    const String& column_name,
    ColumnReader::Visibility visibility /* = ColumnReader::Visibility::SHARED */) {
  auto col_iter = columns_by_name_.find(column_name);
  if (col_iter == columns_by_name_.end()) {
    RAISEF(kNotFoundError, "column not found: $0", column_name);
  }

  auto col_idx = col_iter->second;

  switch (visibility) {

    case ColumnReader::Visibility::SHARED: {
      auto& slot = column_readers_shared_[col_idx];
      if (!slot.get()) {
        slot = column_reader_factories_[col_idx]();
      }

      return slot;
    }

    case ColumnReader::Visibility::PRIVATE: {
      auto reader = column_reader_factories_[col_idx]();
      column_readers_private_.emplace_back(reader);
      return reader;
    }

  }
}

ColumnEncoding CSTableReader::getColumnEncoding(const String& column_name) {
  auto col = getColumnReader(column_name);
  return col->encoding();
}

ColumnType CSTableReader::getColumnType(const String& column_name) {
  auto col = getColumnReader(column_name);
  return col->type();
}

const Vector<ColumnConfig>& CSTableReader::columns() const {
  return columns_;
}

bool CSTableReader::hasColumn(const String& column_name) const {
  auto col = columns_by_name_.find(column_name);
  return col != columns_by_name_.end();
}

size_t CSTableReader::numRecords() const {
  return num_rows_;
}


} // namespace cstable


