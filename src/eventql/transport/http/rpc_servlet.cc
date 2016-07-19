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
#include "eventql/util/util/binarymessagewriter.h"
#include "eventql/transport/http/rpc_servlet.h"
#include "eventql/db/RecordEnvelope.pb.h"
#include "eventql/util/json/json.h"
#include <eventql/util/wallclock.h>
#include <eventql/util/thread/wakeup.h>
#include "eventql/util/protobuf/MessageEncoder.h"
#include "eventql/util/protobuf/MessagePrinter.h"
#include "eventql/util/protobuf/msg.h"
#include <eventql/util/util/Base64.h>
#include <eventql/util/fnv.h>
#include <eventql/io/sstable/sstablereader.h>

#include "eventql/eventql.h"

namespace eventql {

RPCServlet::RPCServlet(
    TableService* node,
    MetadataService* metadata_service,
    const String& tmpdir) :
    node_(node),
    metadata_service_(metadata_service),
    tmpdir_(tmpdir) {}

void RPCServlet::handleHTTPRequest(
    RefPtr<http::HTTPRequestStream> req_stream,
    RefPtr<http::HTTPResponseStream> res_stream) {
  const auto& req = req_stream->request();
  URI uri(req.uri());

  logDebug("eventql", "HTTP Request: $0 $1", req.method(), req.uri());

  http::HTTPResponse res;
  res.populateFromRequest(req);

  res.addHeader("Access-Control-Allow-Origin", "*");
  res.addHeader("Access-Control-Allow-Methods", "GET, POST");
  res.addHeader("Access-Control-Allow-Headers", "X-TSDB-Namespace");

  if (req.method() == http::HTTPMessage::M_OPTIONS) {
    req_stream->readBody();
    res.setStatus(http::kStatusOK);
    res_stream->writeResponse(res);
    return;
  }

  try {
    if (uri.path() == "/tsdb/insert") {
      req_stream->readBody();
      insertRecords(&req, &res, &uri);
      res_stream->writeResponse(res);
      return;
    }

    if (uri.path() == "/tsdb/replicate") {
      req_stream->readBody();
      replicateRecords(&req, &res, &uri);
      res_stream->writeResponse(res);
      return;
    }

    if (uri.path() == "/tsdb/commit") {
      req_stream->readBody();
      commitPartition(&req, &res, &uri);
      res_stream->writeResponse(res);
      return;
    }

    if (uri.path() == "/tsdb/compact") {
      req_stream->readBody();
      compactPartition(&req, &res, &uri);
      res_stream->writeResponse(res);
      return;
    }

    if (uri.path() == "/tsdb/stream") {
      req_stream->readBody();
      streamPartition(&req, &res, res_stream, &uri);
      return;
    }

    if (uri.path() == "/tsdb/update_cstable") {
      updateCSTable(uri, req_stream.get(), &res);
      res_stream->writeResponse(res);
      return;
    }

    if (uri.path() == "/rpc/create_metadata_file") {
      req_stream->readBody();
      createMetadataFile(uri, &req, &res);
      res_stream->writeResponse(res);
      return;
    }

    if (uri.path() == "/rpc/perform_metadata_operation") {
      req_stream->readBody();
      performMetadataOperation(uri, &req, &res);
      res_stream->writeResponse(res);
      return;
    }

    if (uri.path() == "/rpc/discover_partition_metadata") {
      req_stream->readBody();
      discoverPartitionMetadata(uri, &req, &res);
      res_stream->writeResponse(res);
      return;
    }

    if (uri.path() == "/rpc/fetch_metadata_file") {
      req_stream->readBody();
      fetchMetadataFile(uri, &req, &res);
      res_stream->writeResponse(res);
      return;
    }

    if (uri.path() == "/rpc/fetch_latest_metadata_file") {
      req_stream->readBody();
      fetchLatestMetadataFile(uri, &req, &res);
      res_stream->writeResponse(res);
      return;
    }

    if (uri.path() == "/rpc/list_partitions") {
      req_stream->readBody();
      listPartitions(uri, &req, &res);
      res_stream->writeResponse(res);
      return;
    }

    if (uri.path() == "/rpc/find_partition") {
      req_stream->readBody();
      findPartition(uri, &req, &res);
      res_stream->writeResponse(res);
      return;
    }

    res.setStatus(http::kStatusNotFound);
    res.addBody("not found");
    res_stream->writeResponse(res);
  } catch (const Exception& e) {
    try {
      req_stream->readBody();
    } catch (...) {
      /* ignore further errors */
    }

    logError("tsdb", e, "error while processing HTTP request");

    res.setStatus(http::kStatusInternalServerError);
    res.addBody(StringUtil::format("error: $0: $1", e.getTypeName(), e.getMessage()));
    res_stream->writeResponse(res);
  }

  res_stream->finishResponse();
}

void RPCServlet::insertRecords(
    const http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri) {
  //auto record_list = msg::decode<RecordEnvelopeList>(req->body());
  //node_->insertRecords(record_list);
  res->addBody("deprecated call");
  res->setStatus(http::kStatusInternalServerError);
}

void RPCServlet::compactPartition(
    const http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri) {
  const auto& params = uri->queryParams();

  String tsdb_namespace;
  if (!URI::getParam(params, "namespace", &tsdb_namespace)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?namespace=... parameter");
    return;
  }

  String table_name;
  if (!URI::getParam(params, "table", &table_name)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?table=... parameter");
    return;
  }

  String partition_key;
  if (!URI::getParam(params, "partition", &partition_key)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?partition=... parameter");
    return;
  }

  node_->compactPartition(
      tsdb_namespace,
      table_name,
      SHA1Hash::fromHexString(partition_key));

  res->setStatus(http::kStatusCreated);
}

void RPCServlet::commitPartition(
    const http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri) {
  const auto& params = uri->queryParams();

  String tsdb_namespace;
  if (!URI::getParam(params, "namespace", &tsdb_namespace)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?namespace=... parameter");
    return;
  }

  String table_name;
  if (!URI::getParam(params, "table", &table_name)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?table=... parameter");
    return;
  }

  String partition_key;
  if (!URI::getParam(params, "partition", &partition_key)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?partition=... parameter");
    return;
  }

  node_->commitPartition(
      tsdb_namespace,
      table_name,
      SHA1Hash::fromHexString(partition_key));

  res->setStatus(http::kStatusCreated);
}

void RPCServlet::replicateRecords(
    const http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri) {
  const auto& params = uri->queryParams();

  String tsdb_namespace;
  if (!URI::getParam(params, "namespace", &tsdb_namespace)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?namespace=... parameter");
    return;
  }

  String table_name;
  if (!URI::getParam(params, "table", &table_name)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?table=... parameter");
    return;
  }

  String partition_key;
  if (!URI::getParam(params, "partition", &partition_key)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?partition=... parameter");
    return;
  }

  ShreddedRecordList records;
  auto body_is = req->getBodyInputStream();
  records.decode(body_is.get());

  node_->insertReplicatedRecords(
      tsdb_namespace,
      table_name,
      SHA1Hash::fromHexString(partition_key),
      records);

  res->setStatus(http::kStatusCreated);
}

void RPCServlet::streamPartition(
    const http::HTTPRequest* req,
    http::HTTPResponse* res,
    RefPtr<http::HTTPResponseStream> res_stream,
    URI* uri) {
  const auto& params = uri->queryParams();

  String tsdb_namespace;
  if (!URI::getParam(params, "namespace", &tsdb_namespace)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?namespace=... parameter");
    res_stream->writeResponse(*res);
    return;
  }

  String table_name;
  if (!URI::getParam(params, "stream", &table_name)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?stream=... parameter");
    res_stream->writeResponse(*res);
    return;
  }

  String partition_key;
  if (!URI::getParam(params, "partition", &partition_key)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?partition=... parameter");
    res_stream->writeResponse(*res);
    return;
  }

  size_t sample_mod = 0;
  size_t sample_idx = 0;
  String sample_str;
  if (URI::getParam(params, "sample", &sample_str)) {
    auto parts = StringUtil::split(sample_str, ":");

    if (parts.size() != 2) {
      res->setStatus(http::kStatusBadRequest);
      res->addBody("invalid ?sample=... parameter, format is <mod>:<idx>");
      res_stream->writeResponse(*res);
    }

    sample_mod = std::stoull(parts[0]);
    sample_idx = std::stoull(parts[1]);
  }

  res->setStatus(http::kStatusOK);
  res->addHeader("Content-Type", "application/octet-stream");
  res->addHeader("Connection", "close");
  res_stream->startResponse(*res);

  node_->fetchPartitionWithSampling(
      tsdb_namespace,
      table_name,
      SHA1Hash::fromHexString(partition_key),
      sample_mod,
      sample_idx,
      [&res_stream] (const Buffer& record) {
    util::BinaryMessageWriter buf;

    if (record.size() > 0) {
      buf.appendUInt64(record.size());
      buf.append(record.data(), record.size());
      res_stream->writeBodyChunk(Buffer(buf.data(), buf.size()));
    }

    res_stream->waitForReader();
  });

  util::BinaryMessageWriter buf;
  buf.appendUInt64(0);
  res_stream->writeBodyChunk(Buffer(buf.data(), buf.size()));

  res_stream->finishResponse();
}

void RPCServlet::updateCSTable(
    const URI& uri,
    http::HTTPRequestStream* req_stream,
    http::HTTPResponse* res) {
  const auto& params = uri.queryParams();

  String tsdb_namespace;
  if (!URI::getParam(params, "namespace", &tsdb_namespace)) {
    RAISE(kRuntimeError, "missing ?namespace=... parameter");
  }

  String table_name;
  if (!URI::getParam(params, "table", &table_name)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("error: missing ?table=... parameter");
    return;
  }

  String partition_key;
  if (!URI::getParam(params, "partition", &partition_key)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?partition=... parameter");
    return;
  }

  String version;
  if (!URI::getParam(params, "version", &version)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("error: missing ?version=... parameter");
    return;
  }

  auto tmpfile_path = FileUtil::joinPaths(
      tmpdir_,
      StringUtil::format("upload_$0.tmp", Random::singleton()->hex128()));

  {
    auto tmpfile = File::openFile(
        tmpfile_path,
        File::O_CREATE | File::O_READ | File::O_WRITE);

    req_stream->readBody([&tmpfile] (const void* data, size_t size) {
      tmpfile.write(data, size);
    });
  }

  node_->updatePartitionCSTable(
      tsdb_namespace,
      table_name,
      SHA1Hash::fromHexString(partition_key),
      tmpfile_path,
      std::stoull(version));

  res->setStatus(http::kStatusCreated);
}

void RPCServlet::createMetadataFile(
    const URI& uri,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  const auto& params = uri.queryParams();

  String db_namespace;
  if (!URI::getParam(params, "namespace", &db_namespace)) {
    RAISE(kRuntimeError, "missing ?namespace=... parameter");
  }

  String table_name;
  if (!URI::getParam(params, "table", &table_name)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("error: missing ?table=... parameter");
    return;
  }

  MetadataFile file;
  auto is = req->getBodyInputStream();
  auto rc = file.decode(is.get());

  if (rc.isSuccess()) {
    rc = metadata_service_->createMetadataFile(
        db_namespace,
        table_name,
        file);
  }

  if (rc.isSuccess()) {
    res->setStatus(http::kStatusCreated);
  } else {
    res->setStatus(http::kStatusInternalServerError);
    res->addBody("ERROR: " + rc.message());
    return;
  }
}

void RPCServlet::performMetadataOperation(
    const URI& uri,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  const auto& params = uri.queryParams();

  String db_namespace;
  if (!URI::getParam(params, "namespace", &db_namespace)) {
    RAISE(kRuntimeError, "missing ?namespace=... parameter");
  }

  String table_name;
  if (!URI::getParam(params, "table", &table_name)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("error: missing ?table=... parameter");
    return;
  }

  MetadataOperation op;
  {
    auto is = req->getBodyInputStream();
    auto rc = op.decode(is.get());
    if (!rc.isSuccess()) {
      res->setStatus(http::kStatusInternalServerError);
      res->addBody("ERROR: " + rc.message());
      return;
    }
  }

  MetadataOperationResult result;
  auto rc = metadata_service_->performMetadataOperation(
      db_namespace,
      table_name,
      op,
      &result);

  if (rc.isSuccess()) {
    res->setStatus(http::kStatusCreated);
    res->addBody(*msg::encode(result));
  } else {
    res->setStatus(http::kStatusInternalServerError);
    res->addBody("ERROR: " + rc.message());
    return;
  }
}

void RPCServlet::discoverPartitionMetadata(
    const URI& uri,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  PartitionDiscoveryRequest request;
  msg::decode(req->body(), &request);

  PartitionDiscoveryResponse response;
  auto rc = metadata_service_->discoverPartition(request, &response);

  if (rc.isSuccess()) {
    res->setStatus(http::kStatusOK);
    res->addBody(*msg::encode(response));
  } else {
    res->setStatus(http::kStatusInternalServerError);
    res->addBody("ERROR: " + rc.message());
    return;
  }
}

void RPCServlet::fetchMetadataFile(
    const URI& uri,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  const auto& params = uri.queryParams();

  String db_namespace;
  if (!URI::getParam(params, "namespace", &db_namespace)) {
    RAISE(kRuntimeError, "missing ?namespace=... parameter");
  }

  String table_name;
  if (!URI::getParam(params, "table", &table_name)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("error: missing ?table=... parameter");
    return;
  }

  String txid;
  if (!URI::getParam(params, "txid", &txid)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("error: missing ?txid=... parameter");
    return;
  }

  RefPtr<MetadataFile> file;
  auto rc = metadata_service_->getMetadataFile(
      db_namespace,
      table_name,
      SHA1Hash::fromHexString(txid),
      &file);

  if (rc.isSuccess()) {
    auto os = res->getBodyOutputStream();
    rc = file->encode(os.get());
  }

  if (rc.isSuccess()) {
    res->setStatus(http::kStatusOK);
  } else {
    res->setStatus(http::kStatusInternalServerError);
    res->addBody("ERROR: " + rc.message());
    return;
  }
}

void RPCServlet::fetchLatestMetadataFile(
    const URI& uri,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  const auto& params = uri.queryParams();

  String db_namespace;
  if (!URI::getParam(params, "namespace", &db_namespace)) {
    RAISE(kRuntimeError, "missing ?namespace=... parameter");
  }

  String table_name;
  if (!URI::getParam(params, "table", &table_name)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("error: missing ?table=... parameter");
    return;
  }

  RefPtr<MetadataFile> file;
  auto rc = metadata_service_->getMetadataFile(
      db_namespace,
      table_name,
      &file);

  if (rc.isSuccess()) {
    auto os = res->getBodyOutputStream();
    rc = file->encode(os.get());
  }

  if (rc.isSuccess()) {
    res->setStatus(http::kStatusOK);
  } else {
    res->setStatus(http::kStatusInternalServerError);
    res->addBody("ERROR: " + rc.message());
    return;
  }
}

void RPCServlet::listPartitions(
    const URI& uri,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  PartitionListRequest request;
  msg::decode(req->body(), &request);

  PartitionListResponse response;
  auto rc = metadata_service_->listPartitions(request, &response);

  if (rc.isSuccess()) {
    res->setStatus(http::kStatusOK);
    res->addBody(*msg::encode(response));
  } else {
    res->setStatus(http::kStatusInternalServerError);
    res->addBody("ERROR: " + rc.message());
    return;
  }
}

void RPCServlet::findPartition(
    const URI& uri,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  PartitionFindRequest request;
  msg::decode(req->body(), &request);

  PartitionFindResponse response;
  auto rc = metadata_service_->findPartition(request, &response);

  if (rc.isSuccess()) {
    res->setStatus(http::kStatusOK);
    res->addBody(*msg::encode(response));
  } else {
    res->setStatus(http::kStatusInternalServerError);
    res->addBody("ERROR: " + rc.message());
    return;
  }
}

}

