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
#include <eventql/transport/http/status_servlet.h>
#include <eventql/db/PartitionReplication.h>
#include <eventql/server/server_stats.h>
#include <eventql/eventql.h>
#include "eventql/util/application.h"
#include "eventql/db/metadata_client.h"
#include "eventql/db/metadata_store.h"

#include "eventql/eventql.h"
namespace eventql {

static const String kStyleSheet = R"(
  <style type="text/css">
    body, table {
      font-size: 14px;
      line-height: 20px;
      font-weight: normal;
      font-style: normal;
      font-family: 'Helvetica Neue', Helvetica, Arial, sans-serif;
      padding: 0;
      margin: 0;
    }

    body {
      padding: 0 30px 30px 30px;
    }

    em {
      font-style: normal;
      font-weight: bold;
    }

    table td {
      padding: 6px 8px;
    }

    table[border="0"] td {
      padding: 6px 8px 6px 0;
    }

    h1 {
      margin: 40px 0 20px 0;
    }

    h2, h3 {
      margin: 30px 0 15px 0;
    }

    .menu {
      border-bottom: 2px solid #000;
      padding: 6px 0;
    }

    .menu a {
      margin-right: 12px;
      color: #06c;
    }
  </style>
)";

static const String kMainMenu = R"(
  <div class="menu">
    <a href="/zstatus/">Dashboard</a>
    <a href="/zstatus/db/">DB</a>
  </div>
)";

StatusServlet::StatusServlet(
    ServerCfg* config,
    PartitionMap* pmap,
    ConfigDirectory* cdir,
    http::HTTPServerStats* http_server_stats,
    http::HTTPClientStats* http_client_stats,
    ReplicationWorker* repl_worker) :
    config_(config),
    pmap_(pmap),
    cdir_(cdir),
    http_server_stats_(http_server_stats),
    http_client_stats_(http_client_stats),
    repl_worker_(repl_worker) {}

void StatusServlet::handleHTTPRequest(
    http::HTTPRequest* request,
    http::HTTPResponse* response) {
  URI url(request->uri());

  static const String kPathPrefix = "/zstatus/";
  auto path_parts = StringUtil::split(
      url.path().substr(kPathPrefix.size()),
      "/");

  if (path_parts.size() == 2 && path_parts[0] == "db") {
    renderNamespacePage(
        path_parts[1],
        request,
        response);

    return;
  }

  if (path_parts.size() == 3 && path_parts[0] == "db") {
    renderTablePage(
        path_parts[1],
        path_parts[2],
        request,
        response);

    return;
  }

  if (path_parts.size() == 4 && path_parts[0] == "db") {
    renderPartitionPage(
        path_parts[1],
        path_parts[2],
        SHA1Hash::fromHexString(path_parts[3]),
        request,
        response);

    return;
  }

  renderDashboard(request, response);
}

void StatusServlet::renderDashboard(
    http::HTTPRequest* request,
    http::HTTPResponse* response) {
    http::HTTPResponse res;
  auto zs = z1stats();
  String html;
  html += kStyleSheet;
  html += kMainMenu;
  html += StringUtil::format(
      "<h1>EventQL $0 ($1)</h1>",
      kVersionString,
      kBuildID);

  html += "<table cellspacing=0 border=1>";
  html += StringUtil::format(
      "<tr><td><em>Version</em></td><td align='left'>$0 &mdash; $1.$2.$3</td></tr>",
      kVersionString,
      kVersionMajor,
      kVersionMinor,
      kVersionPatch);
  html += StringUtil::format(
      "<tr><td><em>Build-ID</em></td><td align='left'>$0</td></tr>",
      kBuildID);
  html += StringUtil::format(
      "<tr><td><em>Memory Usage - Current</em></td><td align='right'>$0 MB</td></tr>",
      Application::getCurrentMemoryUsage() / (1024.0 * 1024.0));
  html += StringUtil::format(
      "<tr><td><em>Memory Usage - Peak</em></td><td align='right'>$0 MB</td></tr>",
      Application::getPeakMemoryUsage() / (1024.0 * 1024.0));
  html += "</table>";

  html += "<h3>Partitions</h3>";
  html += "<table cellspacing=0 border=1>";
  html += StringUtil::format(
      "<tr><td><em>Number of Partitions</em></td><td align='right'>$0</td></tr>",
      zs->num_partitions.get());
  html += StringUtil::format(
      "<tr><td><em>Number of Partitions - Loaded</em></td><td align='right'>$0</td></tr>",
      zs->num_partitions_loaded.get());
  html += StringUtil::format(
      "<tr><td><em>Number of Dirty Partitions</em></td><td align='right'>$0</td></tr>",
      zs->compaction_queue_length.get());
  html += StringUtil::format(
      "<tr><td><em>LSMTableIndexCache size</em></td><td align='right'>$0 MB</td></tr>",
      config_->idx_cache->size() / (1024.0 * 1024.0));
  html += StringUtil::format(
      "<tr><td><em>MetadataStore cache size</em></td><td align='right'>$0 MB</td></tr>",
      config_->metadata_store->getCacheSize() / (1024.0 * 1024.0));
  html += "</table>";

  html += "<h3>Replication</h3>";
  html += "<table cellspacing=0 border=1>";
  html += StringUtil::format(
      "<tr><td><em>Replication Queue Length</em></td><td align='right'>$0</td></tr>",
      zs->replication_queue_length.get());

  auto num_replication_threads = repl_worker_->getNumThreads();
  for (size_t i = 0; i < num_replication_threads; ++i) {
    html += StringUtil::format(
        "<tr><td><em>Replication Thread #$0</em></td><td>$1</td></tr>",
        i + 1,
        repl_worker_->getReplicationInfo(i)->toString());
  }
  html += "</table>";

  html += "<h3>HTTP</h3>";
  html += "<table cellspacing=0 border=1>";
  html += StringUtil::format(
      "<tr><td><em>Server Connections - Current</em></td><td align='right'>$0</td></tr>",
      http_server_stats_->current_connections.get());
  html += StringUtil::format(
      "<tr><td><em>Server Connections - Total</em></td><td align='right'>$0</td></tr>",
      http_server_stats_->total_connections.get());
  html += StringUtil::format(
      "<tr><td><em>Server Requests - Current</em></td><td align='right'>$0</td></tr>",
      http_server_stats_->current_requests.get());
  html += StringUtil::format(
      "<tr><td><em>Server Requests - Total</em></td><td align='right'>$0</td></tr>",
      http_server_stats_->total_requests.get());
  html += StringUtil::format(
      "<tr><td><em>Server Bytes Received</em></td><td align='right'>$0 MB</td></tr>",
      http_server_stats_->received_bytes.get() / (1024.0 * 1024.0));
  html += StringUtil::format(
      "<tr><td><em>Server Bytes Sent</em></td><td align='right'>$0 MB</td></tr>",
      http_server_stats_->sent_bytes.get() / (1024.0 * 1024.0));
  html += StringUtil::format(
      "<tr><td><em>Client Connections - Current</em></td><td align='right'>$0</td></tr>",
      http_client_stats_->current_connections.get());
  html += StringUtil::format(
      "<tr><td><em>Client Connections - Total</em></td><td align='right'>$0</td></tr>",
      http_client_stats_->total_connections.get());
  html += StringUtil::format(
      "<tr><td><em>Client Requests - Current</em></td><td align='right'>$0</td></tr>",
      http_client_stats_->current_requests.get());
  html += StringUtil::format(
      "<tr><td><em>Client Requests - Total</em></td><td align='right'>$0</td></tr>",
      http_client_stats_->total_requests.get());
  html += StringUtil::format(
      "<tr><td><em>Client Bytes Received</em></td><td align='right'>$0 MB</td></tr>",
      http_client_stats_->received_bytes.get() / (1024.0 * 1024.0));
  html += StringUtil::format(
      "<tr><td><em>Client Bytes Sent</em></td><td align='right'>$0 MB</td></tr>",
      http_client_stats_->sent_bytes.get() / (1024.0 * 1024.0));
  html += "</table>";

  html += "<h3>MapReduce</h3>";
  html += "<table cellspacing=0 border=1>";
  html += StringUtil::format(
      "<tr><td><em>Number of Map Tasks - Running</em></td><td align='right'>$0</td></tr>",
      zs->mapreduce_num_map_tasks.get());
  html += StringUtil::format(
      "<tr><td><em>Number of Reduce Tasks - Running</em></td><td align='right'>$0</td></tr>",
      zs->mapreduce_num_reduce_tasks.get());
  html += StringUtil::format(
      "<tr><td><em>Reduce Memory Used</em></td><td align='right'>$0 MB</td></tr>",
      zs->mapreduce_reduce_memory.get() / (1024.0 * 1024.0));
  html += "</table>";

  html += "<h3>Servers</h3>";
  html += "<table cellspacing=0 border=1>";
  for (const auto& server : cdir_->listServers()) {
  html += StringUtil::format(
      "<tr><td><em>$0</em></td><td><pre>$1</pre></td></tr>",
      server.server_id(),
      server.DebugString());
  }
  html += "</table>";

  html += "<h3>Cluster Config</h3>";
  html += StringUtil::format(
      "<table cellspacing=0 border=1><tr><td><pre>$0</pre></td></tr></table>",
      cdir_->getClusterConfig().DebugString());

  response->setStatus(http::kStatusOK);
  response->addHeader("Content-Type", "text/html; charset=utf-8");
  response->addBody(html);
}

void StatusServlet::renderNamespacePage(
    const String& db_namespace,
    http::HTTPRequest* request,
    http::HTTPResponse* response) {
  String html;
  html += kStyleSheet;
  html += kMainMenu;

  html += StringUtil::format(
      "<h2>Namespace: &nbsp; <span style='font-weight:normal'>$0</span></h2>",
      db_namespace);

  response->setStatus(http::kStatusOK);
  response->addHeader("Content-Type", "text/html; charset=utf-8");
  response->addBody(html);
}

void StatusServlet::renderTablePage(
    const String& db_namespace,
    const String& table_name,
    http::HTTPRequest* request,
    http::HTTPResponse* response) {
  String html;
  html += kStyleSheet;
  html += kMainMenu;

  html += StringUtil::format(
      "<h2>Table: &nbsp; <span style='font-weight:normal'>"
      "<a href='/zstatus/db/$0'>$0</a> &mdash;"
      "$1</span></h2>",
      db_namespace,
      table_name);

  auto table = pmap_->findTable(
      db_namespace,
      table_name);

  if (table.isEmpty()) {
    response->setStatus(http::kStatusNotFound);
    response->addHeader("Content-Type", "text/html; charset=utf-8");
    response->addBody("ERROR: table not found!");
    return;
  }

  const auto& table_cfg = table.get()->config();
  html += StringUtil::format(
      "<span><em>Metatdata TXNID:</em> $0 [$1]</span> &mdash; ",
      SHA1Hash(
          table_cfg.metadata_txnid().data(),
          table_cfg.metadata_txnid().size()).toString(),
      table_cfg.metadata_txnseq());

  MetadataClient metadata_client(cdir_);
  MetadataFile metadata_file;
  auto rc = metadata_client.fetchLatestMetadataFile(
      db_namespace,
      table_name,
      &metadata_file);

  if (!rc.isSuccess()) {
    html += StringUtil::format(
        "<b>ERROR: while fetching metadata: $0</b>",
        rc.message());
  } else {
    html += "<h3>Partition Map:</h3>";
    html += "<table cellspacing=0 border=1>";
    html += "<thead><tr><td>Keyrange</td><td>Partition ID</td><td>Servers</td><td></td></tr></thead>";
    for (const auto& e : metadata_file.getPartitionMap()) {
      Vector<String> servers;
      for (const auto& s : e.servers) {
        servers.emplace_back(s.server_id);
      }
      for (const auto& s : e.servers_joining) {
        servers.emplace_back(s.server_id + " [JOINING]");
      }
      for (const auto& s : e.servers_leaving) {
        servers.emplace_back(s.server_id + " [LEAVING]");
      }

      String keyrange;
      switch (metadata_file.getKeyspaceType()) {
        case KEYSPACE_UINT64: {
          uint64_t keyrange_uint = -1;
          memcpy((char*) &keyrange_uint, e.begin.data(), sizeof(e.begin));
          keyrange = StringUtil::format(
              "$0 [$1]",
              UnixTime(keyrange_uint),
              keyrange_uint);
          break;
        }
        case KEYSPACE_STRING: {
          keyrange = e.begin;
          break;
        }
      }

      String extra_info;
      if (e.splitting) {
        auto split_point = decodePartitionKey(
            table.get()->getKeyspaceType(),
            e.split_point);

        Set<String> servers_low;
        for (const auto& s : e.split_servers_low) {
          servers_low.emplace(s.server_id);
        }
        Set<String> servers_high;
        for (const auto& s : e.split_servers_high) {
          servers_high.emplace(s.server_id);
        }

        extra_info += StringUtil::format(
            "SPLITTING @ $0 into $1 on $2, $3 on $4",
            decodePartitionKey(table.get()->getKeyspaceType(), e.split_point),
            e.split_partition_id_low,
            inspect(servers_low),
            e.split_partition_id_high,
            inspect(servers_high));
      }

      html += StringUtil::format(
          "<tr><td>$0</td><td>$1</td><td>$2</td><td>$3</td></tr>",
          keyrange,
          e.partition_id.toString(),
          StringUtil::join(servers, ", "),
          extra_info);
    }
    html += "</table>";
  }

  html += "<h3>TableDefinition</h3>";
  html += StringUtil::format(
      "<pre>$0</pre>",
      table.get()->config().DebugString());
  html += StringUtil::format(
      "<pre>$0</pre>",
      table.get()->schema()->toString());

  response->setStatus(http::kStatusOK);
  response->addHeader("Content-Type", "text/html; charset=utf-8");
  response->addBody(html);
}

void StatusServlet::renderPartitionPage(
    const String& db_namespace,
    const String& table_name,
    const SHA1Hash& partition_key,
    http::HTTPRequest* request,
    http::HTTPResponse* response) {
  String html;
  html += kStyleSheet;
  html += kMainMenu;

  html += StringUtil::format(
      "<h2>Partition: &nbsp; <span style='font-weight:normal'>"
      "<a href='/zstatus/db/$0'>$0</a> &mdash;"
      "<a href='/zstatus/db/$0/$1'>$1</a> &mdash;"
      "$2</span></h2>",
      db_namespace,
      table_name,
      partition_key.toString());

  auto partition = pmap_->findPartition(
      db_namespace,
      table_name,
      partition_key);

  if (partition.isEmpty()) {
    html += "ERROR: PARTITION NOT FOUND!";
  } else {
    auto table = partition.get()->getTable();
    auto snap = partition.get()->getSnapshot();
    auto state = snap->state;
    auto repl = partition.get()->getReplicationStrategy(nullptr, nullptr);

    if (table->partitionerType() == TBL_PARTITION_TIMEWINDOW &&
        state.partition_keyrange_begin().size() == 8 &&
        state.partition_keyrange_end().size() == 8) {
      uint64_t ts_begin;
      uint64_t ts_end;
      memcpy((char*) &ts_begin, state.partition_keyrange_begin().data(), 8);
      memcpy((char*) &ts_end, state.partition_keyrange_end().data(), 8);

      html += StringUtil::format(
          "<span><em>Keyrange:</em> $0 [$1] - $2 [$3]</span> &mdash; ",
          UnixTime(ts_begin),
          ts_begin,
          UnixTime(ts_end),
          ts_end);
    }

    if (table->partitionerType() == TBL_PARTITION_UINT64 &&
        state.partition_keyrange_begin().size() == 8 &&
        state.partition_keyrange_end().size() == 8) {
      uint64_t range_begin;
      uint64_t range_end;
      memcpy((char*) &range_begin, state.partition_keyrange_begin().data(), 8);
      memcpy((char*) &range_end, state.partition_keyrange_end().data(), 8);

      html += StringUtil::format(
          "<span><em>Keyrange:</em> $0 - $1</span> &mdash; ",
          range_begin,
          range_end);
    }

    if (table->partitionerType() == TBL_PARTITION_STRING) {
      html += StringUtil::format(
          "<span><em>Keyrange:</em> $0 - $1</span> &mdash; ",
          state.partition_keyrange_begin(),
          state.partition_keyrange_end());
    }

    html += StringUtil::format(
        "<span><em>Lifecycle State</em>: $0</span> &mdash; ",
        PartitionLifecycleState_Name(snap->state.lifecycle_state()));

    html += StringUtil::format(
        "<span><em>Needs Replication?</em>: $0</span> &mdash; ",
        repl->needsReplication());

    html += StringUtil::format(
        "<span><em>Splitting?</em>: $0</span> &mdash; ",
        partition.get()->isSplitting());

    html += StringUtil::format(
        "<span><em>Joining Servers?</em>: $0</span> &mdash; ",
        snap->state.has_joining_servers());

    html += StringUtil::format(
        "<span><em>Size (Disk)</em>: $0MB</span> &mdash; ",
        partition.get()->getTotalDiskSize() / 1024.0 / 1024.0);

    html += "<h3>PartitionInfo</h3>";
    html += StringUtil::format(
        "<pre>$0</pre>",
        partition.get()->getInfo().DebugString());

    html += "<h3>PartitionState</h3>";
    html += StringUtil::format("<pre>$0</pre>", snap->state.DebugString());

    html += "<h3>TableDefinition</h3>";
    html += StringUtil::format(
        "<pre>$0</pre>",
        partition.get()->getTable()->config().DebugString());
  }

  response->setStatus(http::kStatusOK);
  response->addHeader("Content-Type", "text/html; charset=utf-8");
  response->addBody(html);
}

}
