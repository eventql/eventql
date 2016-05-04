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
#include <cstable/cstable.h>
#include <cstable/io/PageManager.h>
#include <cstable/io/PageIndex.h>
#include <cstable/io/PageWriter.h>

namespace cstable {

class UInt64PageWriter : public UnsignedIntPageWriter {
public:
  static const uint64_t kPageSize = 512 * 2;

  UInt64PageWriter(
      PageIndexKey key,
      RefPtr<PageManager> page_mgr,
      RefPtr<PageIndex> page_idx);

  void appendValue(uint64_t value) override;

  void writeIndex(OutputStream* os) const override;

protected:
  PageIndexKey key_;
  RefPtr<PageManager> page_mgr_;
  bool has_page_;
  cstable::PageRef page_;
  Buffer page_buf_;
  stx::BufferOutputStream page_os_;
  Vector<Pair<cstable::PageRef, uint64_t>> pages_;
};

} // namespace cstable


