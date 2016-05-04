/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_SSTABLE_SSTableEditor_H
#define _FNORD_SSTABLE_SSTableEditor_H
#include <stdlib.h>
#include <string>
#include <vector>
#include <memory>
#include <stx/io/file.h>
#include <stx/io/pagemanager.h>
#include <eventql/infra/sstable/cursor.h>
#include <eventql/infra/sstable/index.h>
#include <eventql/infra/sstable/indexprovider.h>
#include <stx/exception.h>

namespace stx {
namespace sstable {

class SSTableColumnWriter;
class SSTableColumnSchema;

/**
 * A SSTableEditor allows writing to and reading from an sstable file at the
 * same time.
 */
class SSTableEditor {
public:
  class SSTableEditorCursor : public sstable::Cursor {
  public:
    SSTableEditorCursor(
        SSTableEditor* table,
        io::MmapPageManager* mmap);

    void seekTo(size_t body_offset) override;
    bool trySeekTo(size_t body_offset) override;
    bool next() override;
    bool valid() override;
    void getKey(void** data, size_t* size) override;
    void getData(void** data, size_t* size) override;
    size_t position() const override;
    size_t nextPosition() override;
  protected:
    std::unique_ptr<io::PageManager::PageRef> getPage();
    SSTableEditor* table_;
    io::MmapPageManager* mmap_;
    size_t pos_;
  };

  /**
   * Create and open a new sstable for writing
   */
  static std::unique_ptr<SSTableEditor> create(
      const std::string& filename,
      IndexProvider index_provider,
      void const* header,
      size_t header_size);

  /**
   * Re-open a partially written sstable for writing
   */
  static std::unique_ptr<SSTableEditor> reopen(
      const std::string& filename,
      IndexProvider index_provider);


  SSTableEditor(const SSTableEditor& other) = delete;
  SSTableEditor& operator=(const SSTableEditor& other) = delete;
  ~SSTableEditor();

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
   * Finalize the sstable (writes out the indexes to disk)
   */
  void finalize();

  /**
   * Get an sstable cursor for the sstable currently being written
   */
  std::unique_ptr<SSTableEditorCursor> getCursor();

  template <typename IndexType>
  IndexType* getIndex() const;

  size_t bodySize() const;
  size_t headerSize() const;

  void writeIndex(uint32_t index_type, void* data, size_t size);
  void writeIndex(uint32_t index_type, const Buffer& buf);

protected:

  SSTableEditor(
      const std::string& filename,
      size_t file_size,
      std::vector<Index::IndexRef>&& indexes);

  void writeHeader(void const* data, size_t size);

private:
  void reopen(size_t file_size);

  std::vector<Index::IndexRef> indexes_;
  std::unique_ptr<io::MmapPageManager> mmap_;
  size_t header_size_; // FIXPAUL make atomic
  size_t body_size_; // FIXPAUL make atomic
  bool finalized_;
};


}
}

#include "SSTableEditor_impl.h"
#endif
