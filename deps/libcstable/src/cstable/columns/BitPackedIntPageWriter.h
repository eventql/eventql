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
#include <stx/util/binarymessagewriter.h>
#include <stx/util/BitPackEncoder.h>
#include <cstable/cstable.h>
#include <cstable/io/PageManager.h>
#include <cstable/io/PageWriter.h>


namespace cstable {

class BitPackedIntPageWriter : public UnsignedIntPageWriter {
public:

  BitPackedIntPageWriter(
      RefPtr<PageManager> page_mgr,
      uint32_t max_value = 0xffffffff);

  void appendValue(uint64_t value) override;
  //void addValue(const void* data, size_t size) override;

protected:
  uint32_t max_value_;
};

} // namespace cstable


