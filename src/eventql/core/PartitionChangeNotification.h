/**
 * This file is part of the "tsdb" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/core/PartitionSnapshot.h>
#include <eventql/core/Table.h>

using namespace stx;

namespace zbase {

struct PartitionChangeNotification : public RefCounted {
  RefPtr<Partition> partition;
};

typedef
    Function<void (RefPtr<PartitionChangeNotification> change)>
    PartitionChangeCallbackFn;

} // namespace tdsb

