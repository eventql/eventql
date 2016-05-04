/**
 * This file is part of the "libcstable" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * libcstable is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/exception.h>
#include <stx/autoref.h>
#include <stx/buffer.h>
#include <stx/io/file.h>
#include <cstable/cstable.h>
#include <cstable/io/PageManager.h>

namespace cstable {

class PageWriter;
class PageReader;

struct PageIndexKey {
  uint32_t column_id;
  PageIndexEntryType entry_type;
};

class PageIndex : public stx::RefCounted {
public:

  PageIndex(
      BinaryFormatVersion version,
      RefPtr<PageManager> page_mgr);

  void addPageWriter(PageIndexKey key, PageWriter* page_writer);

  PageRef write(Option<PageRef> head);

protected:
  BinaryFormatVersion version_;
  RefPtr<PageManager> page_mgr_;
  Vector<Pair<PageIndexKey, PageWriter*>> page_writers_;
};

class PageIndexReader {
public:

  PageIndexReader(
      BinaryFormatVersion version,
      RefPtr<PageManager> page_mgr);

  void addPageReader(PageIndexKey key, PageReader* page_reader);

protected:
  BinaryFormatVersion version_;
  RefPtr<PageManager> page_mgr_;
  Vector<Pair<PageIndexKey, PageReader*>> page_readers_;
};

} // namespace cstable

