/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/buffer.h>
#include <eventql/util/SHA1.h>

using namespace stx;

namespace zbase {

struct RecordRef {
  RecordRef(
      const SHA1Hash& _record_id,
      uint64_t _record_version,
      const Buffer& _record);

  SHA1Hash record_id;
  uint64_t record_version;
  Buffer record;
  bool is_update;
};

} // namespace tdsb

