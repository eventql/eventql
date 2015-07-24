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
#include <fnord/stdtypes.h>
#include <fnord/protobuf/MessageSchema.h>
#include <tsdb/TableConfig.pb.h>

using namespace fnord;

namespace tsdb {

struct TSDBTableInfo {
  String table_name;
  RefPtr<msg::MessageSchema> schema;
  TableConfig config;
};

}
