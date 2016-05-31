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
#pragma once
#include "eventql/eventql.h"
#include "eventql/util/status.h"
#include "eventql/util/thread/queue.h"
#include "eventql/db/metadata_operation.h"
#include "eventql/db/metadata_file.h"
#include "eventql/config/config_directory.h"

namespace eventql {

class MetadataReplication {
public:

  MetadataReplication(
      ConfigDirectory* cdir,
      const String& server_name);

  void start();
  void stop();

protected:

  struct ReplicationJob {
    String db_namespace;
    String table_id;
    SHA1Hash transaction_id;
  };

  void applyTableConfigChange(const TableDefinition& cfg);
  void replicate(const ReplicationJob& job);

  ConfigDirectory* cdir_;
  String server_name_;
  std::atomic<bool> running_;
  Vector<std::thread> threads_;
  thread::Queue<ReplicationJob> queue_;
};

} // namespace eventql
