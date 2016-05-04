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
#include <stx/buffer.h>
#include <stx/io/inputstream.h>
#include <stx/io/outputstream.h>
#include <eventql/infra/sstable/MetaPage.h>

namespace stx {
namespace sstable {

struct RowWriter {

  /**
   * Write the provided row to the output stream and return the number of
   * bytes written
   */
  static size_t appendRow(
      const MetaPage& hdr,
      void const* key,
      uint32_t key_size,
      void const* data,
      uint32_t data_size,
      OutputStream* os);

};

}
}
