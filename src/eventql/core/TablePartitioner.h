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
#include <eventql/core/TSDBTableRef.h>
#include <csql/qtree/SequentialScanNode.h>

using namespace stx;

namespace zbase {

class TablePartitioner : public RefCounted {
public:

  virtual SHA1Hash partitionKeyFor(const String& partition_key) const = 0;

  Vector<SHA1Hash> listPartitions() const {
    return listPartitions(Vector<csql::ScanConstraint>{});
  }

  virtual Vector<SHA1Hash> listPartitions(
      const Vector<csql::ScanConstraint>& constraints) const = 0;

};

}
