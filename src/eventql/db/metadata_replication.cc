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
#include "eventql/db/metadata_replication.h"
#include "eventql/util/application.h"

namespace eventql {

MetadataReplication::MetadataReplication(
    ConfigDirectory* cdir,
    const String& server_name) :
    cdir_(cdir),
    server_name_(server_name) {
  cdir_->setTableConfigChangeCallback([this] (const TableDefinition& tbl) {
    applyTableConfigChange(tbl);
  });

  cdir_->listTables([this] (const TableDefinition& tbl) {
    applyTableConfigChange(tbl);
  });
}

void MetadataReplication::applyTableConfigChange(const TableDefinition& cfg) {
  for (const auto& s : cfg.metadata_servers()) {
    if (s == server_name_) {
      ReplicationJob job;
      job.db_namespace = cfg.customer();
      job.table_id = cfg.table_name();
      job.transaction_id = SHA1Hash(
          cfg.metadata_txnid().data(),
          cfg.metadata_txnid().size());

      queue_.insert(job);
      break;
    }
  }
}

void MetadataReplication::replicate(const ReplicationJob& job) {
  iputs("replicate!!", 1);
}

void MetadataReplication::start() {
  running_ = true;

  for (int i = 0; i < 4; ++i) {
    threads_.emplace_back([this] () {
      Application::setCurrentThreadName("evqld-metadatareplication");
      while (running_) {
        auto job = queue_.interruptiblePop();

        try {
          if (!job.isEmpty()) {
            replicate(job.get());
          }
        } catch (const std::exception& e) {
          logWarning("evqld", "error in metadata replication: $0", e.what());
        }
      }
    });
  }
}

void MetadataReplication::stop() {
  if (!running_) {
    return;
  }

  queue_.waitUntilEmpty();
  running_ = false;
  queue_.wakeup();

  for (auto& t : threads_) {
    t.join();
  }
}


} // namespace eventql
