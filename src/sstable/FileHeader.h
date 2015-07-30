/**
 * This file is part of the "libsstable" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>

namespace stx {
namespace sstable {

class FileHeader {
  friend class FieleHeaderReader;
public:

  /**
   * Returns the size of the header, including userdata in bytes
   */
  size_t headerSize() const;

  /**
   * Returns true iff the table is finalized (immutable)
   */
  bool isFinalized() const;

  /**
   * Returns the body size in bytes
   */
  size_t bodySize() const;

  /**
   * Returns the header userdata size in bytes
   */
  size_t userdataSize() const;

  /**
   * Returns the file offset of the header userdata in bytes
   */
  size_t userdataOffset() const;

protected:
  uint64_t flags_;
  uint64_t body_size_;
  uint32_t userdata_checksum_;
  uint32_t userdata_size_;
};

}
}
