/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include <eventql/core/TSDBTableRef.h>
#include <eventql/util/uri.h>
#include <eventql/util/wallclock.h>
#include <eventql/util/human.h>

using namespace stx;

namespace zbase {

TSDBTableRef TSDBTableRef::parse(const String& table_ref) {
  TSDBTableRef ref;

  static const String prefix = "tsdb://";
  if (!StringUtil::beginsWith(table_ref, prefix)) {
    ref.table_key = table_ref;
  } else {
    URI uri(table_ref);
    ref.host = Some(uri.hostAndPort());

    auto parts = StringUtil::split(uri.path().substr(1), "/");

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
      auto now = WallClock::unixMicros();
      auto timespec = suffix.substr(last_prefix.size());
      auto offset = Human::parseDuration(timespec);
      if (!offset.isEmpty()) {
        ref.timerange_begin = Some(UnixTime(now - offset.get().microseconds()));
        ref.timerange_limit = Some(UnixTime(now));
        ref.table_key = ref.table_key.substr(
            0,
            ref.table_key.size() - suffix.size() - 1);
      }
    }
  }

  return ref;
}

} // namespace csql
