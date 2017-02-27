/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include <iostream>
#include <unistd.h>
#include "cluster.h"
#include "eventql/util/io/fileutil.h"
#include "eventql/util/random.h"
#include "../test_runner.h"

namespace eventql {
namespace test {

void startStandaloneCluster(TestContext* ctx) {
  std::string datadir = FileUtil::joinPaths(
      ctx->tmpdir,
      "__evql_test_" + Random::singleton()->hex64());

  FileUtil::mkdir_p(datadir);

  std::unique_ptr<Process> evqld_proc(new Process());
  evqld_proc->logDebug("evqld-standalone", ctx->log_fd);
  evqld_proc->start(
      FileUtil::joinPaths(ctx->bindir, "evqld"),
      std::vector<std::string> {
        "--standalone",
        "--datadir", datadir
      });

  ctx->background_procs["evqld-standalone"] = std::move(evqld_proc);
  usleep(500000); // FIXME give the process time to start
}

} // namespace test
} // namespace eventql

