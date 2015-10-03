/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "stx/io/filerepository.h"
#include "stx/io/fileutil.h"
#include "stx/application.h"
#include "stx/logging.h"
#include "stx/random.h"
#include "stx/thread/eventloop.h"
#include "stx/thread/threadpool.h"
#include "stx/thread/FixedSizeThreadPool.h"
#include "stx/wallclock.h"
#include "stx/VFS.h"
#include "stx/cli/flagparser.h"
#include "stx/json/json.h"
#include "stx/json/jsonrpc.h"
#include "stx/http/httprouter.h"
#include "stx/http/httpserver.h"
#include "stx/http/httpconnectionpool.h"
#include "stx/http/VFSFileServlet.h"
#include "stx/cli/CLI.h"
#include "stx/cli/flagparser.h"
#include "zbase/dproc/LocalScheduler.h"
#include "zbase/dproc/DispatchService.h"
#include "zbase/ConfigDirectory.h"
#include "sstable/sstablereader.h"
#include "zbase/JoinedSession.pb.h"
#include "zbase/core/TimeWindowPartitioner.h"
#include "zbase/core/TSDBClient.h"
#include "csql/qtree/SequentialScanNode.h"
#include "csql/qtree/ColumnReferenceNode.h"
#include "csql/qtree/CallExpressionNode.h"
#include "csql/qtree/GroupByNode.h"
#include "csql/qtree/UnionNode.h"
#include "csql/runtime/queryplanbuilder.h"
#include "csql/CSTableScan.h"
#include "csql/CSTableScanProvider.h"
#include "csql/runtime/defaultruntime.h"
#include "csql/runtime/tablerepository.h"

using namespace stx;
using namespace zbase;

stx::thread::EventLoop ev;

void cmd_cluster_status(const cli::FlagParser& flags) {
  ConfigDirectoryClient cclient(
      InetAddr::resolve(flags.getString("master")));

  auto cluster = cclient.fetchClusterConfig();
  iputs("Cluster config:\n$0", cluster.DebugString());
}

void cmd_cluster_add_node(const cli::FlagParser& flags) {
  ConfigDirectoryClient cclient(
      InetAddr::resolve(flags.getString("master")));

  auto cluster = cclient.fetchClusterConfig();
  cclient.updateClusterConfig(cluster);
}

void cmd_import_session_sstable(const cli::FlagParser& flags) {
  thread::EventLoop ev;

  auto evloop_thread = std::thread([&ev] {
    ev.run();
  });

  stx::logInfo(
      "analyticsctl",
      "Uploading sstable: $0",
      flags.getString("sstable"));

  http::HTTPConnectionPool http(&ev);
  zbase::TSDBClient tsdb("http://localhost:8000/tsdb", &http);

  sstable::SSTableReader reader(flags.getString("sstable"));
  auto cursor = reader.getCursor();

  auto batch_size = 100;
  zbase::RecordEnvelopeList batch;
  auto upload_batch = [&tsdb, &batch] {
    stx::logInfo(
        "analyticsctl",
        "Uploading $0 records...",
        batch.records_size());

    try {
      tsdb.insertRecords(batch);
      batch.clear_records();
    } catch (const Exception& e) {
      stx::logError("analyticsctl", e, "error while uploading records");
    }
  };

  size_t nrecs = 0;
  while (cursor->valid()) {
    auto record_id = SHA1::compute(cursor->getKeyBuffer());
    auto record_data = cursor->getDataBuffer();

    auto session = msg::decode<zbase::JoinedSession>(record_data);
    auto time = UnixTime(session.first_seen_time() * kMicrosPerSecond);
    if (time.unixMicros() == 0) {
      RAISE(kRuntimeError, "session has no time");
    }

    auto tsdb_ns = "dawanda";
    auto table_name = "logjoin.joined_sessions";
    auto partition_key = zbase::TimeWindowPartitioner::partitionKeyFor(
        table_name,
        time,
        4 * kMicrosPerHour);

    auto r = batch.add_records();
    r->set_tsdb_namespace(tsdb_ns);
    r->set_table_name(table_name);
    r->set_partition_sha1(partition_key.toString());
    r->set_record_id(record_id.toString());
    r->set_record_data(record_data.toString());

    if (batch.records_size() >= batch_size) {
      upload_batch();
    }

    ++nrecs;

    if (!cursor->next()) {
      break;
    }
  }

  upload_batch();

  stx::logInfo(
      "analyticsctl",
      "Finished uploading sstable: $0, $1 records",
      flags.getString("sstable"),
      nrecs);

  ev.shutdown();
  evloop_thread.join();
}

void cmd_backfill_session_sstable(const cli::FlagParser& flags) {
  thread::EventLoop ev;

  auto evloop_thread = std::thread([&ev] {
    ev.run();
  });

  stx::logInfo(
      "analyticsctl",
      "Uploading sstable: $0",
      flags.getString("sstable"));

  http::HTTPConnectionPool http(&ev);
  zbase::TSDBClient tsdb("http://nue05.prod.fnrd.net:7004/tsdb", &http);

  sstable::SSTableReader reader(flags.getString("sstable"));
  auto cursor = reader.getCursor();

  auto batch_size = 100;
  zbase::RecordEnvelopeList batch;
  auto upload_batch = [&tsdb, &batch] {
    stx::logInfo(
        "analyticsctl",
        "Uploading $0 records...",
        batch.records_size());

    try {
      tsdb.insertRecords(batch);
      batch.clear_records();
    } catch (const Exception& e) {
      stx::logError("analyticsctl", e, "error while uploading records");
    }
  };

  size_t nrecs = 0;
  auto tsdb_ns = "dawanda";
  auto table_name = String("sessions");
  while (cursor->valid()) {
    auto record_id = SHA1::compute(cursor->getKeyBuffer());
    auto record_data = cursor->getDataBuffer();

    auto old_session = msg::decode<zbase::JoinedSession>(record_data);
    if (old_session.last_seen_time() == 0) {
      RAISE(kRuntimeError, "last seen time is empty");
    }

    zbase::NewJoinedSession new_session;

    size_t n = 0;
    for (const auto& old_ev : old_session.search_queries()) {
      auto new_ev = new_session.mutable_event()->add_search_query();
      *new_ev = old_ev;

      new_ev->set_time(old_ev.time() * kMicrosPerSecond);

      auto evkey = table_name + ".search_query";
      auto evid = SHA1::compute(
          record_id.toString() + StringUtil::toString(++n));
      auto partition_key = zbase::TimeWindowPartitioner::partitionKeyFor(
          evkey,
          old_ev.time() * kMicrosPerSecond,
          4 * kMicrosPerHour);

      auto r = batch.add_records();
      r->set_tsdb_namespace(tsdb_ns);
      r->set_table_name(evkey);
      r->set_partition_sha1(partition_key.toString());
      r->set_record_id(evid.toString());
      r->set_record_data(msg::encode(*new_ev)->toString());
    }

    for (const auto& old_ev : old_session.page_views()) {
      auto new_ev = new_session.mutable_event()->add_page_view();
      *new_ev = old_ev;

      new_ev->set_time(old_ev.time() * kMicrosPerSecond);

      auto evkey = table_name + ".page_view";
      auto evid = SHA1::compute(
          record_id.toString() + StringUtil::toString(++n));
      auto partition_key = zbase::TimeWindowPartitioner::partitionKeyFor(
          evkey,
          old_ev.time() * kMicrosPerSecond,
          4 * kMicrosPerHour);

      auto r = batch.add_records();
      r->set_tsdb_namespace(tsdb_ns);
      r->set_table_name(evkey);
      r->set_partition_sha1(partition_key.toString());
      r->set_record_id(evid.toString());
      r->set_record_data(msg::encode(*new_ev)->toString());
    }

    for (const auto& old_ev : old_session.cart_items()) {
      auto new_ev = new_session.mutable_event()->add_cart_items();
      *new_ev = old_ev;

      new_ev->set_time(old_ev.time() * kMicrosPerSecond);

      auto evkey = table_name + ".cart_items";
      auto evid = SHA1::compute(
          record_id.toString() + StringUtil::toString(++n));
      auto partition_key = zbase::TimeWindowPartitioner::partitionKeyFor(
          evkey,
          old_ev.time() * kMicrosPerSecond,
          4 * kMicrosPerHour);

      auto r = batch.add_records();
      r->set_tsdb_namespace(tsdb_ns);
      r->set_table_name(evkey);
      r->set_partition_sha1(partition_key.toString());
      r->set_record_id(evid.toString());
      r->set_record_data(msg::encode(*new_ev)->toString());
    }

    new_session.set_session_id(SHA1::compute(cursor->getKeyString()).toString());
    new_session.set_time(old_session.last_seen_time() * kMicrosPerSecond);
    new_session.set_first_seen_time(old_session.first_seen_time() * kMicrosPerSecond);
    new_session.set_last_seen_time(old_session.last_seen_time() * kMicrosPerSecond);

    new_session.mutable_attr()->set_customer_session_id(old_session.customer_session_id());
    new_session.mutable_attr()->set_referrer_url(old_session.referrer_url());
    new_session.mutable_attr()->set_referrer_campaign(old_session.referrer_campaign());
    new_session.mutable_attr()->set_referrer_name(old_session.referrer_name());
    new_session.mutable_attr()->set_num_cart_items(old_session.num_cart_items());
    new_session.mutable_attr()->set_num_order_items(old_session.num_order_items());
    new_session.mutable_attr()->set_cart_value_eurcents(old_session.cart_value_eurcents());
    new_session.mutable_attr()->set_gmv_eurcents(old_session.gmv_eurcents());
    new_session.mutable_attr()->set_ab_test_group(old_session.ab_test_group());

    {
      auto time = UnixTime(old_session.last_seen_time() * kMicrosPerSecond);
      auto partition_key = zbase::TimeWindowPartitioner::partitionKeyFor(
          table_name,
          time,
          4 * kMicrosPerHour);

      auto r = batch.add_records();
      r->set_tsdb_namespace(tsdb_ns);
      r->set_table_name(table_name);
      r->set_partition_sha1(partition_key.toString());
      r->set_record_id(record_id.toString());
      r->set_record_data(msg::encode(new_session)->toString());
    }

    if (batch.records_size() >= batch_size) {
      upload_batch();
    }

    ++nrecs;

    if (!cursor->next()) {
      break;
    }
  }

  upload_batch();

  stx::logInfo(
      "analyticsctl",
      "Finished uploading sstable: $0, $1 records",
      flags.getString("sstable"),
      nrecs);

  ev.shutdown();
  evloop_thread.join();
}

int main(int argc, const char** argv) {
  stx::Application::init();
  stx::Application::logToStderr();

  stx::cli::FlagParser flags;

  flags.defineFlag(
      "loglevel",
      stx::cli::FlagParser::T_STRING,
      false,
      NULL,
      "INFO",
      "loglevel",
      "<level>");

  flags.parseArgv(argc, argv);

  Logger::get()->setMinimumLogLevel(
      strToLogLevel(flags.getString("loglevel")));

  cli::CLI cli;

  /* command: import */
  auto import_cmd = cli.defineCommand("import_session_sstable");
  import_cmd->onCall(std::bind(&cmd_import_session_sstable, std::placeholders::_1));

  import_cmd->flags().defineFlag(
      "sstable",
      stx::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "input file path",
      "<path>");

  /* command: backfill */
  auto backfill_cmd = cli.defineCommand("backfill_session_sstable");
  backfill_cmd->onCall(std::bind(&cmd_backfill_session_sstable, std::placeholders::_1));

  backfill_cmd->flags().defineFlag(
      "sstable",
      stx::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "input file path",
      "<path>");

  /* command: cluster_status */
  auto cluster_status_cmd = cli.defineCommand("cluster_status");
  cluster_status_cmd->onCall(
      std::bind(&cmd_cluster_status, std::placeholders::_1));

  cluster_status_cmd->flags().defineFlag(
      "master",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "url",
      "<addr>");

  /* command: cluster_add_node */
  auto cluster_add_node_cmd = cli.defineCommand("cluster_add_node");
  cluster_add_node_cmd->onCall(
      std::bind(&cmd_cluster_add_node, std::placeholders::_1));

  cluster_add_node_cmd->flags().defineFlag(
      "master",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "url",
      "<addr>");

  cluster_add_node_cmd->flags().defineFlag(
      "name",
      stx::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "node name",
      "<string>");

  cli.call(flags.getArgv());
  return 0;
}

