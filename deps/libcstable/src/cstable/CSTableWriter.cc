/**
 * This file is part of the "libcstable" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * libcstable is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <cstable/CSTableWriter.h>
#include <cstable/columns/UnsignedIntColumnWriter.h>
#include <cstable/columns/v1/BooleanColumnWriter.h>
#include <cstable/columns/v1/BitPackedIntColumnWriter.h>
#include <cstable/columns/v1/UInt32ColumnWriter.h>
#include <cstable/columns/v1/UInt64ColumnWriter.h>
#include <cstable/columns/v1/LEB128ColumnWriter.h>
#include <cstable/columns/v1/DoubleColumnWriter.h>
#include <cstable/columns/v1/StringColumnWriter.h>
#include <stx/SHA1.h>
#include <stx/io/fileutil.h>
#include <stx/option.h>

using namespace stx;

namespace cstable {

RefPtr<CSTableWriter> CSTableWriter::createFile(
    const String& filename,
    const TableSchema& schema,
    Option<RefPtr<LockRef>> lockref /* = None<RefPtr<LockRef>>() */) {
  return createFile(
      filename,
      BinaryFormatVersion::v0_2_0,
      schema,
      lockref);
}

RefPtr<CSTableWriter> CSTableWriter::createFile(
    const String& filename,
    BinaryFormatVersion version,
    const TableSchema& schema,
    Option<RefPtr<LockRef>> lockref /* = None<RefPtr<LockRef>>() */) {
  auto file = File::openFile(filename, File::O_WRITE | File::O_CREATE);
  auto file_os = FileOutputStream::fromFileDescriptor(file.fd());

  FileHeader header;
  header.schema = mkRef(new TableSchema(schema));
  header.columns = header.schema->flatColumns();

  // write header
  size_t header_size ;
  switch (version) {
    case BinaryFormatVersion::v0_1_0:
      header_size = 0;
      break;
    case BinaryFormatVersion::v0_2_0:
      header_size = cstable::v0_2_0::writeHeader(header, file_os.get());
      break;
  }

  auto page_mgr = new PageManager(
      version,
      std::move(file),
      header_size);

  return new CSTableWriter(
      version,
      header.schema,
      page_mgr,
      new PageIndex(version, page_mgr),
      header.columns);
}

RefPtr<CSTableWriter> CSTableWriter::reopenFile(
    const String& filename,
    Option<RefPtr<LockRef>> lockref /* = None<RefPtr<LockRef>>() */) {
  RAISE(kNotYetImplementedError);
}

static RefPtr<ColumnWriter> openColumnV1(const ColumnConfig& c) {
  auto rmax = c.rlevel_max;
  auto dmax = c.dlevel_max;

  switch (c.storage_type) {
    case ColumnEncoding::BOOLEAN_BITPACKED:
      return new v1::BooleanColumnWriter(rmax, dmax);
    case ColumnEncoding::UINT32_BITPACKED:
      return new v1::BitPackedIntColumnWriter(rmax, dmax);
    case ColumnEncoding::UINT32_PLAIN:
      return new v1::UInt32ColumnWriter(rmax, dmax, c.logical_type);
    case ColumnEncoding::UINT64_PLAIN:
      return new v1::UInt64ColumnWriter(rmax, dmax, c.logical_type);
    case ColumnEncoding::UINT64_LEB128:
      return new v1::LEB128ColumnWriter(rmax, dmax, c.logical_type);
    case ColumnEncoding::FLOAT_IEEE754:
      return new v1::DoubleColumnWriter(rmax, dmax);
    case ColumnEncoding::STRING_PLAIN:
      return new v1::StringColumnWriter(rmax, dmax);
    default:
      RAISEF(
          kRuntimeError,
          "unsupported column type: $0",
          (uint32_t) c.storage_type);
  }
}

static RefPtr<ColumnWriter> openColumnV2(
    const ColumnConfig& c,
    RefPtr<PageManager> page_mgr,
    RefPtr<PageIndex> page_idx) {
  switch (c.logical_type) {
    //case ColumnType::BOOLEAN:
    //  return new BooleanColumnWriter(c, page_mgr, page_idx);
    case ColumnType::UNSIGNED_INT:
      return new UnsignedIntColumnWriter(c, page_mgr, page_idx);
    //case ColumnType::SIGNED_INT:
    //  return new SignedIntColumnWriter(c, page_mgr, page_idx);
    //case ColumnType::STRING:
    //  return new StringColumnWriter(c, page_mgr, page_idx);
    //case ColumnType::FLOAT:
    //  return new FloatColumnWriter(c, page_mgr, page_idx);
    //case ColumnType::DATETIME:
    //  return new DateTimeColumnWriter(c, page_mgr, page_idx);
  }
}

CSTableWriter::CSTableWriter(
    BinaryFormatVersion version,
    RefPtr<TableSchema> schema,
    RefPtr<PageManager> page_mgr,
    RefPtr<PageIndex> page_idx,
    Vector<ColumnConfig> columns) :
    version_(version),
    schema_(std::move(schema)),
    page_mgr_(page_mgr),
    page_idx_(page_idx),
    columns_(columns),
    current_txid_(0),
    num_rows_(0) {

  // open column writers
  for (size_t i = 0; i < columns_.size(); ++i) {
    RefPtr<ColumnWriter> writer;
    switch (version_) {
      case BinaryFormatVersion::v0_1_0:
        writer = openColumnV1(columns_[i]);
        break;
      case BinaryFormatVersion::v0_2_0:
        writer = openColumnV2(columns_[i], page_mgr_, page_idx_);
        break;
    }

    column_writers_.emplace_back(writer);
    column_writers_by_name_.emplace(columns_[i].column_name, writer);
  }
}

void CSTableWriter::addRow() {
  num_rows_++;
}

void CSTableWriter::addRows(size_t num_rows) {
  num_rows_ += num_rows;
}

void CSTableWriter::commit() {
  switch (version_) {
    case BinaryFormatVersion::v0_1_0:
      return commitV1();
    case BinaryFormatVersion::v0_2_0:
      return commitV2();
  }
}

void CSTableWriter::commitV1() {
  for (size_t i = 0; i < columns_.size(); ++i) {
    auto writer = column_writers_[i].asInstanceOf<v1::ColumnWriter>();
    writer->commit();
    columns_[i].body_size = writer->bodySize();
  }

  stx::util::BinaryMessageWriter header;
  header.appendUInt32(cstable::v0_1_0::kMagicBytesUInt32);
  header.appendUInt16(cstable::v0_1_0::kVersion);
  header.appendUInt64(0); // flags
  header.appendUInt64(num_rows_);
  header.appendUInt32(columns_.size());

  size_t offset = header.size();
  for (const auto& col : columns_) {
    offset += 32 + col.column_name.size();
  }

  for (auto& col : columns_) {
    col.body_offset = offset;
    offset += col.body_size;

    header.appendUInt32((uint32_t) col.storage_type);
    header.appendUInt32(col.column_name.length());
    header.append(col.column_name.data(), col.column_name.length());
    header.appendUInt32(col.rlevel_max);
    header.appendUInt32(col.dlevel_max);
    header.appendUInt64(col.body_offset);
    header.appendUInt64(col.body_size);
  }

  page_mgr_->writePage(0, header.size(), header.data(), header.size());

  for (size_t i = 0; i < columns_.size(); ++i) {
    const auto& col = columns_[i];
    auto writer = column_writers_[i].asInstanceOf<v1::ColumnWriter>();

    Buffer buf(col.body_size);
    writer->write(buf.data(), buf.size());
    RCHECK(buf.size() == col.body_size, "invalid column body size");

    page_mgr_->writePage(
        col.body_offset,
        col.body_size,
        buf.data(),
        buf.size());
  }
}

void CSTableWriter::commitV2() {
  // write new index
  auto idx_head = page_idx_->write(free_idx_ptr_);

  // build new meta block
  MetaBlock mb;
  mb.transaction_id = current_txid_ + 1;
  mb.num_rows = num_rows_;
  mb.head_index_page_offset = idx_head.offset;
  mb.head_index_page_size = idx_head.size;
  mb.file_size = page_mgr_->getOffset();

  // commit tx to disk
  page_mgr_->writeTransaction(mb);
  free_idx_ptr_ = cur_idx_ptr_;
  cur_idx_ptr_ = Some(idx_head);
}

RefPtr<ColumnWriter> CSTableWriter::getColumnWriter(
    const String& column_name) const {
  auto col = column_writers_by_name_.find(column_name);
  if (col == column_writers_by_name_.end()) {
    RAISEF(kNotFoundError, "column not found: $0", column_name);
  }

  return col->second;
}

bool CSTableWriter::hasColumn(const String& column_name) const {
  auto col = column_writers_by_name_.find(column_name);
  return col != column_writers_by_name_.end();
}

const TableSchema* CSTableWriter::schema() {
  return schema_.get();
}

const Vector<ColumnConfig>& CSTableWriter::columns() const {
  return columns_;
}

} // namespace cstable


