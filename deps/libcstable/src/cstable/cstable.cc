/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <cstable/cstable.h>
#include <cstable/io/PageManager.h>
#include <stx/util/binarymessagewriter.h>
#include <stx/util/binarymessagereader.h>
#include <stx/SHA1.h>

using namespace stx;

namespace cstable {

void readHeader(
    BinaryFormatVersion* version,
    FileHeader* header,
    MetaBlock* metablock,
    Option<PageRef>* free_index,
    InputStream* is) {
  Buffer prelude(6);
  is->readNextBytes(prelude.data(), prelude.size());

  if (memcmp(prelude.structAt<void>(0), kMagicBytes, 4) != 0) {
    RAISE(kRuntimeError, "not a valid cstable file");
  }

  Vector<MetaBlock> metablocks;
  auto vnum = *prelude.structAt<uint8_t>(4);
  switch (vnum) {
    case 0x1:
      *version = BinaryFormatVersion::v0_1_0;
      cstable::v0_1_0::readHeader(header, is);
      return;
    case 0x2:
      *version = BinaryFormatVersion::v0_2_0;
      cstable::v0_2_0::readHeader(header, &metablocks, is);
      break;
    default:
      RAISEF(kRuntimeError, "unsupported cstable version: $0", vnum);
  }

  switch (metablocks.size()) {

    case 0:
      RAISE(kRuntimeError, "can't open cstable: no valid metablocks found");

    case 1:
      *metablock = metablocks[0];
      *free_index = None<PageRef>();
      break;

    case 2:
      if (metablocks[0].transaction_id > metablocks[1].transaction_id) {
        *metablock = metablocks[0];
        *free_index = Some(PageRef {
          .offset = metablocks[1].head_index_page_offset,
          .size = metablocks[1].head_index_page_size
        });
      } else {
        *metablock = metablocks[1];
        *free_index = Some(PageRef {
          .offset = metablocks[0].head_index_page_offset,
          .size = metablocks[0].head_index_page_size
        });
      }
      break;

    default:
      RAISE(kRuntimeError, "can't open cstable: too many metablocks found");
  }

}

namespace v0_1_0 {

void readHeader(FileHeader* hdr, InputStream* is) {
  auto flags = is->readUInt64();
  (void) flags; // make gcc happy

  hdr->num_rows = is->readUInt64();

  auto ncols = is->readUInt32();
  for (size_t i = 0; i < ncols; ++i) {
    ColumnConfig col;
    col.storage_type = (cstable::ColumnEncoding) is->readUInt32();
    col.column_name = is->readString(is->readUInt32());
    col.column_id = 0;
    col.rlevel_max = is->readUInt32();
    col.dlevel_max = is->readUInt32();
    col.body_offset = is->readUInt64();
    col.body_size = is->readUInt64();
    hdr->columns.emplace_back(col);
  }

  std::sort(
      hdr->columns.begin(),
      hdr->columns.end(),
      [] (const ColumnConfig& a, const ColumnConfig& b) {
    return a.column_name < b.column_name;
  });
}

} // namespace v0_1_0

namespace v0_2_0 {

size_t writeMetaBlock(const MetaBlock& mb, OutputStream* os) {
  stx::util::BinaryMessageWriter buf;
  buf.appendUInt64(mb.transaction_id); // transaction id
  buf.appendUInt64(mb.num_rows); // number of rows
  buf.appendUInt64(mb.head_index_page_offset); // head index page offset
  buf.appendUInt32(mb.head_index_page_size); // head index page size
  buf.appendUInt64(mb.file_size); // file size in bytes
  os->write((char*) buf.data(), buf.size());

  auto hash = SHA1::compute(buf.data(), buf.size());
  os->write((char*) hash.data(), hash.size()); // sha1 hash

  return buf.size() + hash.size();
}

bool readMetaBlock(MetaBlock* mb, InputStream* is) {
  Buffer buf(kMetaBlockSize);
  is->readNextBytes(buf.data(), kMetaBlockSize);

  auto hash_a = SHA1::compute(buf.data(), kMetaBlockSize - SHA1Hash::kSize);
  auto hash_b = SHA1Hash(
      buf.structAt<void>(kMetaBlockSize - SHA1Hash::kSize),
      SHA1Hash::kSize);

  if (hash_a == hash_b) {
    stx::util::BinaryMessageReader reader(buf.data(), buf.size());
    mb->transaction_id = *reader.readUInt64();
    mb->num_rows = *reader.readUInt64();
    mb->head_index_page_offset = *reader.readUInt64();
    mb->head_index_page_size = *reader.readUInt32();
    mb->file_size = *reader.readUInt64();
    return true;
  } else {
    return false;
  }
}

size_t writeHeader(const FileHeader& hdr, OutputStream* os) {
  stx::util::BinaryMessageWriter buf;
  buf.append(kMagicBytes, sizeof(kMagicBytes));
  buf.appendUInt16(2); // version
  buf.appendUInt64(0); // flags
  RCHECK(buf.size() == kMetaBlockPosition, "invalid meta block offset");
  buf.appendString(String(kMetaBlockSize * 2, '\0')); // empty meta blocks
  buf.appendString(String(128, '\0')); // 128 bytes reserved
  buf.appendVarUInt(hdr.columns.size());

  for (const auto& col : hdr.columns) {
    buf.appendVarUInt((uint8_t) col.logical_type);
    buf.appendVarUInt((uint8_t) col.storage_type);
    buf.appendVarUInt(col.column_id);
    buf.appendLenencString(col.column_name);
    buf.appendVarUInt(col.rlevel_max);
    buf.appendVarUInt(col.dlevel_max);
  }

  // pad header to next 512 byte boundary
  auto header_padding = padToNextSector(buf.size()) - buf.size();
  buf.appendString(String(header_padding, '\0'));

  os->write((const char*) buf.data(), buf.size());
  return buf.size();
}

void readHeader(
    FileHeader* hdr,
    Vector<MetaBlock>* metablocks,
    InputStream* is) {
  auto flags = is->readUInt64();
  (void) flags; // make gcc happy

  for (size_t i = 0; i < 2; ++i) {
    MetaBlock mb;
    if (readMetaBlock(&mb, is)) {
      metablocks->emplace_back(mb);
    }
  }

  is->skipNextBytes(128);

  auto ncols = is->readVarUInt();
  for (size_t i = 0; i < ncols; ++i) {
    ColumnConfig col;
    col.logical_type = (cstable::ColumnType) is->readVarUInt();
    col.storage_type = (cstable::ColumnEncoding) is->readVarUInt();
    col.column_id = is->readVarUInt();
    col.column_name = is->readLenencString();
    col.rlevel_max = is->readVarUInt();
    col.dlevel_max = is->readVarUInt();
    hdr->columns.emplace_back(col);
  }
}

} // namespace v0_2_0

} // namespace cstable
