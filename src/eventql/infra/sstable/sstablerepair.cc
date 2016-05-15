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
#include <eventql/util/exception.h>
#include <eventql/util/fnv.h>
#include <eventql/infra/sstable/sstablereader.h>
#include <eventql/infra/sstable/sstablerepair.h>

using stx::Exception;

namespace stx {
namespace sstable {

SSTableRepair::SSTableRepair(
    const std::string& filename) :
    filename_(filename) {}

bool SSTableRepair::checkAndRepair(bool repair /* = false */) {
  auto file = File::openFile(filename_, File::O_READ);
  std::unique_ptr<stx::sstable::SSTableReader> reader_;

  try {
    reader_.reset(new sstable::SSTableReader(std::move(file)));
  } catch (stx::Exception& rte) {
    /*
    fnordmetric::env()->logger()->printf(
        "INFO",
        "SSTableRepair: sstable %s header is corrupt: %s",
        filename_.c_str(),
        rte.getMessage().c_str());
    */

    /* the constructor raises if the checksum is invalid or if the file
     * metadata exceeds the file bounds. there is nothing we can do to recover
     * if that happens */
    return false;
  }

  if (reader_->bodySize() == 0) {
    return checkAndRepairUnfinishedTable(repair);
  } else {
    try {
      reader_->readFooter(0);
    } catch (stx::Exception& rte) {
      /*
      fnordmetric::env()->logger()->printf(
          "INFO",
          "SSTableRepair: sstable %s footer is corrupt: %s",
          filename_.c_str(),
          rte.getMessage().c_str());
      */

      return false;
    }
  }

  return true;
}

bool SSTableRepair::checkAndRepairUnfinishedTable(bool repair) {
  io::MmappedFile file(File::openFile(filename_, File::O_READ));
  FileHeaderReader header_reader(file.data(), file.size());

  if (!header_reader.verify()) {
    return false;
  }

  auto pos = header_reader.headerSize();
  auto end = file.size();

  while (pos < end) {
    if (pos + sizeof(BinaryFormat::RowHeader) > end) {
      break;
    }

    auto row_header = file.structAt<BinaryFormat::RowHeader>(pos);
    auto row_len = sizeof(BinaryFormat::RowHeader) +
        row_header->key_size + row_header->data_size;

    if (pos + row_len > end) {
      break;
    }

    FNV<uint32_t> fnv;
    auto checksum = fnv.hash(
        file.structAt<void>(pos + sizeof(uint32_t)),
        row_len - sizeof(uint32_t));

    if (row_header->checksum != checksum) {
      break;
    }

    pos += row_len;
  }

  if (pos < end) {
    /*
    fnordmetric::env()->logger()->printf(
        "INFO",
        "SSTableRepair: found %i extraneous trailing bytes in sstable %s",
        (int) (end - pos),
        filename_.c_str());
    */

    if (repair) {
      /*
      fnordmetric::env()->logger()->printf(
          "INFO",
          "SSTableRepair: truncating sstable %s to %i bytes",
          filename_.c_str(),
          (int) pos);
      */

      auto writable_file = File::openFile(filename_, File::O_WRITE);
      writable_file.truncate(pos);
    } else {
      return false;
    }
  }

  return true;
}

}
}

