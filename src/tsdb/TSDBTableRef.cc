/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <tsdb/TSDBTableRef.h>
#include <fnord/uri.h>

using namespace fnord;

namespace tsdb {

TSDBTableRef TSDBTableRef::parse(const String& table_ref) {
  TSDBTableRef ref;

  static const String prefix = "tsdb://";
  if (!StringUtil::beginsWith(table_ref, prefix)) {
    ref.table_key = table_ref;
  } else {
    URI uri(table_ref);
    ref.host = Some(uri.hostAndPort());

    auto parts = StringUtil::split(uri.path(), "/");

    switch (parts.size()) {

      case 2:
        ref.partition_key = SHA1Hash::fromHexString(parts[1]);
        /* fallthrough */

      case 1:
        ref.table_key = parts[0];
        break;

      default:
        RAISEF(
            kIllegalArgumentError,
            "invalid tsdb table reference '$0', format is: "
            "tsdb://<host>/<table>[/<partition>]",
            table_ref);

    }
  }

  auto spos = StringUtil::findLast(ref.table_key, '.');
  if (spos != String::npos) {
    auto suffix = ref.table_key.substr(spos + 1);

    static const String last_prefix = "last";
    if (StringUtil::beginsWith(suffix, last_prefix)) {
      auto timespec = suffix.substr(last_prefix.size());
      fnord::iputs("ts: $0", timespec);
    }
  }

  return ref;
}

} // namespace csql
