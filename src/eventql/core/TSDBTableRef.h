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
#include <eventql/util/UnixTime.h>
#include <eventql/util/option.h>
#include <eventql/util/SHA1.h>

using namespace stx;

namespace zbase {

struct TSDBTableRef {
  static TSDBTableRef parse(const String& table_ref);

  String table_key;
  Option<String> host;
  Option<SHA1Hash> partition_key;
  Option<UnixTime> timerange_begin;
  Option<UnixTime> timerange_limit;
};

} // namespace csql
