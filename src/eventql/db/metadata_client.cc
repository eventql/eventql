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
#include "eventql/db/metadata_client.h"
#include <eventql/util/logging.h>

namespace eventql {

MetadataClient::MetadataClient(ConfigDirectory* cdir) : cdir_(cdir) {}

Status MetadataClient::fetchLatestMetadataFile(
    const String& ns,
    const String& table_id,
    MetadataFile* file) {
  auto table_cfg = cdir_->getTableConfig(ns, table_id);

  http::HTTPClient http_client;
  for (const auto& s : table_cfg.metadata_servers()) {
    auto server = cdir_->getServerConfig(s);
    if (server.server_status() != SERVER_UP) {
      continue;
    }

    auto url = StringUtil::format(
        "http://$0/rpc/fetch_latest_metadata_file?namespace=$1&table=$2",
        server.server_addr(),
        URI::urlEncode(ns),
        URI::urlEncode(table_id));

    Buffer body;
    auto req = http::HTTPRequest::mkPost(url, body);
    //auth_->signRequest(static_cast<Session*>(txn_->getUserData()), &req);

    http::HTTPResponse res;
    auto rc = http_client.executeRequest(req, &res);
    if (!rc.isSuccess()) {
      logWarning("evqld", "metadata fetch failed: $0", rc.message());
      continue;
    }

    if (res.statusCode() == 200) {
      auto is = res.getBodyInputStream();
      return file->decode(is.get());
    } else {
      logWarning(
          "evqld",
          "metadata fetch failed: $0",
          res.body().toString());
    }
  }

  return Status(eIOError, "no metadata server responded");
}

Status MetadataClient::fetchMetadataFile(
    const TableDefinition& table_config,
    MetadataFile* file) {
  return fetchMetadataFile(
      table_config.customer(),
      table_config.table_name(),
      SHA1Hash(
          table_config.metadata_txnid().data(),
          table_config.metadata_txnid().size()),
      file);
}

Status MetadataClient::fetchMetadataFile(
    const String& ns,
    const String& table_id,
    const SHA1Hash& txnid,
    MetadataFile* file) {
  auto table_cfg = cdir_->getTableConfig(ns, table_id);

  http::HTTPClient http_client;
  for (const auto& s : table_cfg.metadata_servers()) {
    auto server = cdir_->getServerConfig(s);
    if (server.server_status() != SERVER_UP) {
      continue;
    }

    auto url = StringUtil::format(
        "http://$0/rpc/fetch_metadata_file?namespace=$1&table=$2&txid=$3",
        server.server_addr(),
        URI::urlEncode(ns),
        URI::urlEncode(table_id),
        txnid);

    Buffer body;
    auto req = http::HTTPRequest::mkPost(url, body);
    //auth_->signRequest(static_cast<Session*>(txn_->getUserData()), &req);

    http::HTTPResponse res;
    auto rc = http_client.executeRequest(req, &res);
    if (!rc.isSuccess()) {
      logWarning("evqld", "metadata fetch failed: $0", rc.message());
      continue;
    }

    if (res.statusCode() == 200) {
      auto is = res.getBodyInputStream();
      return file->decode(is.get());
    } else {
      logWarning(
          "evqld",
          "metadata fetch failed: $0",
          res.body().toString());
    }
  }

  return Status(eIOError, "no metadata server responded");
}

Status MetadataClient::listPartitions(
    const String& ns,
    const String& table_id,
    const KeyRange& keyrange,
    PartitionListResponse* res) {
  auto table_cfg = cdir_->getTableConfig(ns, table_id);

  PartitionListRequest req;
  req.set_db_namespace(ns);
  req.set_table_id(table_id);
  req.set_keyrange_begin(keyrange.begin);
  req.set_keyrange_end(keyrange.end);

  http::HTTPClient http_client;
  for (const auto& s : table_cfg.metadata_servers()) {
    auto server = cdir_->getServerConfig(s);
    if (server.server_status() != SERVER_UP) {
      continue;
    }

    auto url = StringUtil::format(
        "http://$0/rpc/list_partitions",
        server.server_addr());

    auto http_req = http::HTTPRequest::mkPost(url, *msg::encode(req));
    //auth_->signRequest(static_cast<Session*>(txn_->getUserData()), &req);

    http::HTTPResponse http_res;
    auto rc = http_client.executeRequest(http_req, &http_res);
    if (!rc.isSuccess()) {
      logWarning("evqld", "metadata request failed: $0", rc.message());
      continue;
    }

    if (http_res.statusCode() == 200) {
      msg::decode<PartitionListResponse>(http_res.body(), res);
      return Status::success();
    } else {
      logWarning(
          "evqld",
          "metadata request failed: $0",
          http_res.body().toString());
    }
  }

  return Status(eIOError, "no metadata server responded");
}

Status MetadataClient::findPartition(
    const String& ns,
    const String& table_id,
    const String& key,
    PartitionFindResponse* res) {
 auto table_cfg = cdir_->getTableConfig(ns, table_id);

  PartitionFindRequest req;
  req.set_db_namespace(ns);
  req.set_table_id(table_id);
  req.set_key(key);

  http::HTTPClient http_client;
  for (const auto& s : table_cfg.metadata_servers()) {
    auto server = cdir_->getServerConfig(s);
    if (server.server_status() != SERVER_UP) {
      continue;
    }

    auto url = StringUtil::format(
        "http://$0/rpc/find_partition",
        server.server_addr());

    auto http_req = http::HTTPRequest::mkPost(url, *msg::encode(req));
    //auth_->signRequest(static_cast<Session*>(txn_->getUserData()), &req);

    http::HTTPResponse http_res;
    auto rc = http_client.executeRequest(http_req, &http_res);
    if (!rc.isSuccess()) {
      logWarning("evqld", "metadata request failed: $0", rc.message());
      continue;
    }

    if (http_res.statusCode() == 200) {
      msg::decode<PartitionFindResponse>(http_res.body(), res);
      return Status::success();
    } else {
      logWarning(
          "evqld",
          "metadata request failed: $0",
          http_res.body().toString());
    }
  }

  return Status(eIOError, "no metadata server responded");

}

} // namespace eventql
