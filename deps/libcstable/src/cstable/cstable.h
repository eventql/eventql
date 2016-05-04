/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stdlib.h>
#include <stdint.h>
#include <stx/io/inputstream.h>
#include <stx/io/outputstream.h>
#include <stx/protobuf/MessageObject.h>

namespace cstable {

static const uint32_t kVersionMajor = 0;
static const uint32_t kVersionMinor = 2;
static const uint32_t kVersionPatch = 0;
static const std::string kVersionString = "v0.2.0-dev";

/**
 * // http://tools.ietf.org/html/rfc5234
 *
 * Assumptions:
 *   - aligned 512 write bytes to disk are atomic
 *
 * v0.1.x:
 *
 *   <cstable> :=
 *       <header>
 *       <body>
 *
 *   <header> :=
 *       %x23 %x17 %x23 %x17     // magic bytes
 *       %x01 %x00               // cstable file format version
 *       <uint64_t>              // flags
 *       <uint64_t>              // number of rows in the table
 *       <uint32_t>              // number of columns
 *       <column_header>*        // column header for each column
 *
 *   <column_header> :=
 *       <uint32_t>              // column type
 *       <uint32_t>              // length of the column name
 *       <char>*                 // column name
 *       <uint32_t>              // max repetition level
 *       <uint32_t>              // max definition level
 *       <uint64_t>              // column data start offset
 *       <uint64_t>              // column data size
 *
 * v0.2.0:
 *
 *   <cstable> :=
 *       <header>
 *       ( <index_page> / data_page )*
 *
 *   <header> :=
 *       %x23 %x17 %x23 %x17     // magic bytes
 *       %x02 %x00               // cstable file format version
 *       <uint64_t>              // flags
 *       <metablock>             // metablock a
 *       <metablock>             // metablock b
 *       <128 bytes>             // reserved
 *       <lenenc_int>            // number of columns
 *       <column_info>*          // column info for each column
 *       %x00*                   // padding to next 512 byte boundary
 *
 *   <metablock> :=
 *       <uint64_t>              // transaction id
 *       <uint64_t>              // number of rows
 *       <uint64_t>              // head index page offset
 *       <uint32_t>              // head index page size
 *       <uint64_t>              // file size in bytes
 *       <20 bytes>              // sha1 checksum
 *
 *   <column_info> :=
 *       <lenenc_int>            // column logical type
 *       <lenenc_int>            // column storage type
 *       <lenenc_int>            // column id
 *       <lenenc_string>         // column name
 *       <lenenc_int>            // max repetition level
 *       <lenenc_int>            // max definition level
 *
 *   <index_page> :=
 *       <uint64_t>              // next index page file offset
 *       <uint32_t>              // next index page size
 *       <uint32_t>              // bytes used in this page
 *       <char*>                 // index page data (index_entries back2back)
 *
 *   <index_entry> :=
 *      <lenenc_int>             // entry type (0x1=data, 0x2=repetition level, 0x3=definition level)
 *      <lenenc_int>             // field id
 *      <char>*                  // column index data
 *
 *  <data_page>       :=
 *      <char>*                  // page data
 *
 */
enum class ColumnType : uint8_t {
  SUBRECORD = 0,
  BOOLEAN = 1,
  UNSIGNED_INT = 2,
  SIGNED_INT = 3,
  STRING = 4,
  FLOAT = 5,
  DATETIME = 6
};

enum class ColumnEncoding : uint8_t {
  BOOLEAN_BITPACKED = 1,
  UINT32_BITPACKED = 10,
  UINT32_PLAIN = 11,
  UINT64_PLAIN = 12,
  UINT64_LEB128 = 13,
  FLOAT_IEEE754 = 14,
  STRING_PLAIN = 100
};

enum class BinaryFormatVersion {
  v0_1_0,
  v0_2_0
};

static const char kMagicBytes[4] = {0x23, 0x17, 0x23, 0x17};

static const size_t kSectorSize = 512;

using String = std::string;
using StringUtil = stx::StringUtil;

template <typename T>
using ScopedPtr = std::unique_ptr<T>;

template <typename T>
using Vector = std::vector<T>;

template <typename T>
using Set = std::set<T>;

template <typename T>
using Function = std::function<T>;

template <typename T1, typename T2>
using Pair = std::pair<T1, T2>;

template <typename T1, typename T2>
using HashMap = std::unordered_map<T1, T2>;

template <typename T>
using RefPtr = stx::RefPtr<T>;
using RefCounted = stx::RefCounted;

template <typename T>
using ScopedPtr = stx::ScopedPtr<T>;

template <typename T>
using Option = stx::Option<T>;

using InputStream = stx::InputStream;
using OutputStream = stx::OutputStream;
using FileOutputStream = stx::FileOutputStream;
using File = stx::File;
using Buffer = stx::Buffer;
using UnixTime = stx::UnixTime;

inline uint64_t padToNextSector(uint64_t val) {
  return (((val) + (kSectorSize - 1)) / kSectorSize) * kSectorSize;
}

String columnTypeToString(ColumnType type);
ColumnType columnTypeFromString(String str);

struct ColumnConfig {
  uint32_t column_id;
  String column_name;
  ColumnEncoding storage_type;
  ColumnType logical_type;
  size_t rlevel_max;
  size_t dlevel_max;
  uint64_t body_offset; // deprecated after v0.1.x
  uint64_t body_size; // deprecated after v0.1.x
};

struct MetaBlock {
  uint64_t transaction_id;
  uint64_t num_rows;
  uint64_t head_index_page_offset;
  uint32_t head_index_page_size;
  uint64_t file_size;
};

} // namespace cstable

#include <cstable/TableSchema.h>

namespace cstable {

struct FileHeader {
  RefPtr<TableSchema> schema;
  Vector<ColumnConfig> columns;
  uint64_t num_rows; // deprecated after v0.1.x
};

struct PageRef;

void readHeader(
    BinaryFormatVersion* version,
    FileHeader* header,
    MetaBlock* metablock,
    Option<PageRef>* free_index,
    InputStream* is);

enum class PageIndexEntryType : uint8_t {
  DATA = 1,
  RLEVEL = 2,
  DLEVEL = 3
};

/* v0.1.0 */
namespace v0_1_0 {
const uint16_t kVersion = 1;
const uint32_t kMagicBytesUInt32 = 0x17231723;
void readHeader(FileHeader* mb, InputStream* is);
}

/* v0.2.0 */
namespace v0_2_0 {
static const uint16_t kVersion = 2;
const size_t kMetaBlockPosition = 14;
const size_t kMetaBlockSize = 56;
size_t writeMetaBlock(const MetaBlock& mb, OutputStream* os);
bool readMetaBlock(MetaBlock* mb, InputStream* is);
size_t writeHeader(const FileHeader& hdr, OutputStream* os);
void readHeader(FileHeader* mb, Vector<MetaBlock>* metablocks, InputStream* is);
}

} // namespace cstable

