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
#include <stx/stdtypes.h>
#include <stx/option.h>
#include <stx/UnixTime.h>
#include <stx/duration.h>
#include <stx/SHA1.h>
#include <tsdb/TSDBTableRef.h>

using namespace stx;

namespace tsdb {

class TablePartitioner : public RefCounted {
public:

  virtual SHA1Hash partitionKeyFor(const String& partition_key) const = 0;

  virtual Vector<SHA1Hash> partitionKeysFor(
      const TSDBTableRef& table_ref) const = 0;

};

}
