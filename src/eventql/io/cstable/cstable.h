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
#include <stdlib.h>
#include <stdint.h>
#include <eventql/util/io/inputstream.h>
#include <eventql/util/io/outputstream.h>
#include <eventql/util/protobuf/MessageObject.h>

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
 *       <uint64_t>              // index page offset
 *       <uint32_t>              // index page size
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
 *      <index_entry>*           // index entries back to back
 *
 *   <index_entry> :=
 *      <lenenc_int>             // entry type (0x1=data, 0x2=repetition level, 0x3=definition level)
 *      <lenenc_int>             // field id
 *      <lenenc_int>             // page offset
 *      <lenenc_int>             // page size
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
  uint64_t index_offset;
  uint32_t index_size;
};

} // namespace cstable

#include <eventql/io/cstable/TableSchema.h>

namespace cstable {

struct FileHeader {
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

struct PageRef {
  uint64_t offset;
  uint32_t size;
};

struct PageIndexKey {
  uint32_t column_id;
  PageIndexEntryType entry_type;
};

struct PageIndexEntry {
  PageIndexKey key;
  PageRef page;
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
const size_t kMetaBlockSize = 48;
size_t writeMetaBlock(const MetaBlock& mb, OutputStream* os);
bool readMetaBlock(MetaBlock* mb, InputStream* is);
size_t writeHeader(const FileHeader& hdr, OutputStream* os);
void readHeader(FileHeader* mb, Vector<MetaBlock>* metablocks, InputStream* is);
size_t writeIndex(const Vector<PageIndexEntry>& index, OutputStream* os);
void readIndex(Vector<PageIndexEntry>* index, InputStream* os);
}

} // namespace cstable

