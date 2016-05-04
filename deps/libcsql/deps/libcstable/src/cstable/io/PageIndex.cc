/**
 * This file is part of the "libcstable" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * libcstable is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <cstable/io/PageIndex.h>
#include <cstable/io/PageWriter.h>
#include <stx/util/binarymessagewriter.h>


namespace cstable {

PageIndex::PageIndex(
    BinaryFormatVersion version,
    RefPtr<PageManager> page_mgr) :
    version_(version),
    page_mgr_(page_mgr) {}

void PageIndex::addPageWriter(PageIndexKey key, PageWriter* page_writer) {
  page_writers_.emplace_back(key, page_writer);
}

PageRef PageIndex::write(Option<PageRef> head) {
  static const size_t kIndexPageSize = 512 * 1024;
  static const size_t kIndexPageOverhead = 16;

  // build index
  Buffer buf;
  {
    auto os = stx::BufferOutputStream::fromBuffer(&buf);
    for (auto& p : page_writers_) {
      const auto& key = p.first;
      auto& writer = p.second;
      os->appendVarUInt((uint8_t) key.entry_type);
      os->appendVarUInt(key.column_id);
      writer->writeIndex(os.get());
    }
  }

  // allocate pages
  Vector<PageRef> pages;
  {
    size_t remaining = buf.size();
    Option<PageRef> next = head;
    while (remaining > 0) {
      if (next.isEmpty()) {
        pages.emplace_back(
            page_mgr_->allocPage(
                std::max(kIndexPageSize, remaining + kIndexPageOverhead)));

        break;
      } else {
        RAISE(kNotImplementedError);
      }
    }
  }

  // write index to disk
  uint64_t written = 0;
  Buffer page_buf;
  auto page_os = stx::BufferOutputStream::fromBuffer(&page_buf);
  for (size_t i = 0; i < pages.size(); ++i) {
    page_buf.clear();

    auto used = std::min(
        uint32_t(pages[i].size - kIndexPageOverhead),
        uint32_t(buf.size() - written));

    if (i + 1 < pages.size()) {
      page_os->appendUInt64(pages[i+1].offset);
      page_os->appendUInt32(pages[i+1].size);
    } else {
      page_os->appendUInt64(0);
      page_os->appendUInt32(0);
    }

    page_os->appendUInt32(used);
    page_os->write(buf.structAt<char>(written), used);

    page_mgr_->writePage(pages[i], page_buf);
    written += used;
  }

  RCHECK(written == buf.size(), "invalid page allocation");

  return pages[0];
}

PageIndexReader::PageIndexReader(
    BinaryFormatVersion version,
    RefPtr<PageManager> page_mgr) :
    version_(version),
    page_mgr_(page_mgr) {}

void PageIndexReader::addPageReader(PageIndexKey key, PageReader* page_reader) {
  page_readers_.emplace_back(key, page_reader);
}


} // namespace cstable


