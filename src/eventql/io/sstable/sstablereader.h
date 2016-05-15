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
#ifndef _FNORD_SSTABLE_SSTABLEREADER_H
#define _FNORD_SSTABLE_SSTABLEREADER_H
#include <stdlib.h>
#include <string>
#include <vector>
#include <memory>
#include <eventql/util/buffer.h>
#include <eventql/util/exception.h>
#include <eventql/util/io/file.h>
#include <eventql/util/io/mmappedfile.h>
#include <eventql/io/sstable/binaryformat.h>
#include <eventql/io/sstable/fileheaderreader.h>
#include <eventql/io/sstable/cursor.h>
#include <eventql/io/sstable/index.h>
#include <eventql/io/sstable/indexprovider.h>


namespace sstable {

class SSTableReader {
public:
  class SSTableReaderCursor : public sstable::Cursor {
  public:
    SSTableReaderCursor(
        RefPtr<RewindableInputStream> is,
        size_t begin,
        size_t limit);

    void seekTo(size_t body_offset) override;
    bool trySeekTo(size_t body_offset) override;
    bool next() override;
    bool valid() override;
    void getKey(void** data, size_t* size) override;
    void getData(void** data, size_t* size) override;
    size_t position() const override;
    size_t nextPosition() override;
  protected:
    bool fetchMeta();
    RefPtr<RewindableInputStream> is_;
    size_t begin_;
    size_t limit_;
    size_t pos_;
    bool valid_;
    bool have_key_;
    Buffer key_;
    size_t key_size_;
    bool have_value_;
    Buffer value_;
    size_t value_size_;
  };

  explicit SSTableReader(const String& filename);
  explicit SSTableReader(File&& file);
  explicit SSTableReader(RefPtr<VFSFile> vfs_file);
  explicit SSTableReader(RefPtr<RewindableInputStream> inputstream);
  SSTableReader(const SSTableReader& other) = delete;
  SSTableReader& operator=(const SSTableReader& other) = delete;

  /**
   * Get an sstable cursor for the body of this sstable
   */
  std::unique_ptr<SSTableReaderCursor> getCursor();

  Buffer readHeader();
  void readFooter(uint32_t type, void** data, size_t* size);
  Buffer readFooter(uint32_t type);

  /**
   * Returns the body size in bytes
   */
  size_t bodySize() const;

  /**
   * Returns true iff the table is finalized
   */
  bool isFinalized() const;

  /**
   * Returns the body offset (the position of the first body byte in the file)
   */
  size_t bodyOffset() const;

  /**
   * Returns the size of the userdata blob stored in the sstable header in bytes
   **/
  size_t headerSize() const;

  /**
   * Returns the number of rows in thos table
   */
  size_t countRows();

private:
  RefPtr<RewindableInputStream> is_;
  MetaPage header_;
};


}

#endif
