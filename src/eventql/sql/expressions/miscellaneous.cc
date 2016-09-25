/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Laura Schlimmer <laura@eventql.io>
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
#include <stdlib.h>
#include <eventql/sql/expressions/miscellaneous.h>
#include <eventql/util/exception.h>

namespace csql {
namespace expressions {

void usleepExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
  if (argc != 1) {
    RAISE(
        kRuntimeError,
        "wrong number of arguments for usleep expected: 1, got: %i", argc);
  }

  auto total_sleep = argv[0].getInteger();
  if (total_sleep < 0) {
    RAISE(
        kRuntimeError,
        "wrong argument for usleep, must be positive integer, got %i",
        total_sleep);
  }

  auto sleep_count = total_sleep / kUSleepTime;
  for (auto i = 0; i < sleep_count; ++i) {
    usleep(kUSleepTime);

    auto rc = ((Transaction*) *ctx)->triggerHeartbeat();
    if (!rc.isSuccess()) {
      *out = SValue::newInteger(1);
      return;
    }
  }

  usleep(total_sleep % kUSleepTime);
  *out = SValue::newInteger(0);
}

} //expressions
} //csql

