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
    const String& server_name,
    MetadataStore* store) :
    cdir_(cdir),
    server_name_(server_name),
    metadata_store_(store) {
  cdir_->setTableConfigChangeCallback([this] (const TableDefinition& tbl) {
    applyTableConfigChange(tbl);
  });

  cdir_->listTables([this] (const TableDefinition& tbl) {
    applyTableConfigChange(tbl);
  });
}

MetadataReplication::~MetadataReplication() {
  if (running_) {
    stop();
  }
}

void MetadataReplication::applyTableConfigChange(const TableDefinition& cfg) {
  bool has_local_replica = false;
  Vector<String> servers;
  for (const auto& s : cfg.metadata_servers()) {
    if (s == server_name_) {
      has_local_replica = true;
    } else {
      servers.emplace_back(s);
    }
  }

  if (!has_local_replica) {
    return;
  }

  ReplicationJob job;
  job.db_namespace = cfg.customer();
  job.table_id = cfg.table_name();
  job.servers = servers;
  job.transaction_id = SHA1Hash(
      cfg.metadata_txnid().data(),
      cfg.metadata_txnid().size());

  queue_.insert(job, WallClock::now());
}

void MetadataReplication::replicateWithRetries(const ReplicationJob& job) {
  auto rc = Status::success();
  try {
    rc = replicate(job);
  } catch (const std::exception& e) {
    rc = Status(eRuntimeError, e.what());
  }

  if (!rc.isSuccess()) {
    logWarning("evqld", "metadata replication failed: $0", rc.message());

    // check that the transaction we are retrying to fetch is still the most
    // recent one
    auto table_cfg = cdir_->getTableConfig(job.db_namespace, job.table_id);
    SHA1Hash head_txnid(
        table_cfg.metadata_txnid().data(),
        table_cfg.metadata_txnid().size());

    if (head_txnid == job.transaction_id) {
      queue_.insert(job, WallClock::unixMicros() + kRetryDelayMicros);
    }
  }
}

Status MetadataReplication::replicate(const ReplicationJob& job) {
  bool has_file = metadata_store_->hasMetadataFile(
      job.db_namespace,
      job.table_id,
      job.transaction_id);
  if (has_file) {
    return Status::success();
  }

  http::HTTPClient http_client;
  for (const auto& s : job.servers) {
    try {
      auto server = cdir_->getServerConfig(s);
      if (server.server_status() != SERVER_UP) {
        continue;
      }

      auto url = StringUtil::format(
          "http://$0/rpc/fetch_metadata_file?namespace=$1&table=$2&txid=$3",
          server.server_addr(),
          URI::urlEncode(job.db_namespace),
          URI::urlEncode(job.table_id),
          job.transaction_id.toString());

      Buffer body;
      auto req = http::HTTPRequest::mkPost(url, body);
      //auth_->signRequest(static_cast<Session*>(txn_->getUserData()), &req);

      http::HTTPResponse res;
      auto rc = http_client.executeRequest(req, &res);
      if (!rc.isSuccess()) {
        logWarning("evqld", "metadata fetch failed: $0", rc.message());
        continue;
      }

      if (res.statusCode() != 200) {
        logWarning(
            "evqld",
            "metadata fetch failed: $0",
            res.body().toString());
        continue;
      }

      auto is = res.getBodyInputStream();
      MetadataFile file;
      file.decode(is.get());

      return metadata_store_->storeMetadataFile(
          job.db_namespace,
          job.table_id,
          file);
    } catch (const std::exception& e) {
      logWarning("evqld", "metadata fetch failed: $0", e.what());
    }
  }

  return Status(eIOError, "no metadata server responded");
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
            replicateWithRetries(job.get());
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

  running_ = false;
  queue_.wakeup();

  for (auto& t : threads_) {
    t.join();
  }
}


} // namespace eventql
