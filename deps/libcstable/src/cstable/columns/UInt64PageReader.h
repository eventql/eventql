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
#include <cstable/io/PageReader.h>


namespace cstable {

class UInt64PageReader : public UnsignedIntPageReader {
public:
  static const uint64_t kPageSize = 512 * 2;

  UInt64PageReader(RefPtr<PageManager> page_mgr);

  uint64_t readUnsignedInt() const override;

  void readIndex(InputStream* os) const override;

protected:
  RefPtr<PageManager> page_mgr_;
  Vector<Pair<cstable::PageRef, uint64_t>> pages_;
};

} // namespace cstable


