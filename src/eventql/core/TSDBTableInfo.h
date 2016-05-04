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
#include <eventql/util/protobuf/MessageSchema.h>
#include <eventql/core/TableConfig.pb.h>

using namespace stx;

namespace zbase {

struct TSDBTableInfo {
  String table_name;
  RefPtr<msg::MessageSchema> schema;
  TableDefinition config;
};

}
