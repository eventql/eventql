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
#include <sys/time.h>
#include <sys/resource.h>
#include <eventql/transport/http/status_servlet.h>
#include <eventql/db/partition_replication.h>
#include <eventql/server/server_stats.h>
#include <eventql/eventql.h>
#include "eventql/util/application.h"
#include "eventql/db/metadata_client.h"
#include "eventql/db/metadata_store.h"
#include "eventql/db/file_tracker.h"
#include "eventql/server/session.h"
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
    <a href="/eventql/">Dashboard</a>
    <a href="/eventql/db/">DB</a>
  </div>
)";

StatusServlet::StatusServlet(Database* db) : db_(db) {}

void StatusServlet::handleHTTPRequest(
    http::HTTPRequest* request,
    http::HTTPResponse* response) {
  URI url(request->uri());

  static const String kPathPrefix = "/eventql/";
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
  auto ctx = db_->getSession()->getDatabaseContext();
  auto cdir = ctx->config_directory;

  auto zs = evqld_stats();
  String html;
  html += kStyleSheet;
  //html += kMainMenu;
  html += StringUtil::format(
      "<h1>EventQL $0 ($1)</h1>",
      kVersionString,
      kBuildID);

  struct rlimit fd_limit;
  memset(&fd_limit, 0, sizeof(fd_limit));
  ::getrlimit(RLIMIT_NOFILE, &fd_limit);

  html += "<pre>";

  html += StringUtil::format("Version: $0\n", kVersionString);
  html += StringUtil::format("Build-ID: $0\n", kBuildID);
  html += StringUtil::format(
      "Memory Usage - Current: $0 MB\n",
      Application::getCurrentMemoryUsage() / (1024.0 * 1024.0));
  html += StringUtil::format(
      "Memory Usage - Peak: $0 MB\n",
      Application::getPeakMemoryUsage() / (1024.0 * 1024.0));
  html += StringUtil::format(
      "Max FDs: $0 (soft) / $1 (hard)\n",
      fd_limit.rlim_cur,
      fd_limit.rlim_max);
  html += StringUtil::format(
      "Referenced Files: $0\n",
      ctx->file_tracker->getNumReferencedFiles());
  html += StringUtil::format(
      "Disk Cache Size: $0 MB\n",
      zs->cache_size.get() / (1024.0 * 1024.0));
  html += StringUtil::format(
      "LSMTableIndexCache size: $0 MB\n",
      ctx->lsm_index_cache->size() / (1024.0 * 1024.0));
  html += StringUtil::format(
      "MetadataStore cache size: $0 MB\n",
      ctx->metadata_store->getCacheSize() / (1024.0 * 1024.0));
  html += StringUtil::format(
      "Number of Partitions: $0\n",
      zs->num_partitions.get());
  html += StringUtil::format(
      "Number of Partitions - Opened: $0\n",
      zs->num_partitions_opened.get());
  html += StringUtil::format(
      "Number of Partitions - Loading: $0\n",
      zs->num_partitions_loading.get());
  html += StringUtil::format(
      "Number of Dirty Partitions: $0\n",
      zs->compaction_queue_length.get());
  html += StringUtil::format(
      "Replication Queue Length: $0</p>",
      zs->replication_queue_length.get());
  html += StringUtil::format(
      "Number of Map Tasks - Running: $0\n",
      zs->mapreduce_num_map_tasks.get());
  html += StringUtil::format(
      "Number of Reduce Tasks - Running: $0\n",
      zs->mapreduce_num_reduce_tasks.get());
  html += StringUtil::format(
      "Reduce Memory Used: $0 MB</p>",
      zs->mapreduce_reduce_memory.get() / (1024.0 * 1024.0));
  html += StringUtil::format(
      "Cluster Leader: $0</p>",
      cdir->getLeader());
  html += "</pre>";

  html += "<h3>Servers</h3>";
  html += "<table cellspacing=0 border=1>";
  html += "<tr><th>Name</th><th>Status</th><th>ListenAddr</th>"
          "<th>BuildInfo</th><th>Disk %</th><th>Disk Used</th><th>Disk Free</th>"
          "<th>Partitions</th></tr>";

  for (const auto& server : cdir->listServers()) {
    if (server.is_dead()) {
      continue;
    }

    const auto& sstats = server.server_stats();
    auto disk_capacity = sstats.disk_used() + sstats.disk_available();

    html += StringUtil::format(
        "<tr><td>$0</td><td>$1</td><td>$2</td><td>$3</td><td>$4</td>"
        "<td>$5</td><td>$6</td><td>$7/$8</td></tr>",
        server.server_id(),
        ServerStatus_Name(server.server_status()),
        server.server_addr(),
        sstats.buildinfo(),
        sstats.has_disk_used() && sstats.has_disk_available() ?
            StringUtil::toString(
                (double) sstats.disk_used() / (double) disk_capacity) :
            "-",
        sstats.has_disk_used() ?
            StringUtil::format("$0MB", sstats.disk_used() / 0x100000) :
            "-",
        sstats.has_disk_available() ?
            StringUtil::format("$0MB", sstats.disk_available() / 0x100000) :
            "-",
        sstats.has_partitions_loaded() ?
            StringUtil::toString(sstats.partitions_loaded()) :
            "-",
        sstats.has_partitions_assigned() ?
            StringUtil::toString(sstats.partitions_assigned()) :
            "-");
  }
  html += "</table>";

  html += "<h3>Background Threads</h3>";
  html += "<table cellspacing=0 border=1>";
  auto num_replication_threads = ctx->replication_worker->getNumThreads();
  for (size_t i = 0; i < num_replication_threads; ++i) {
    html += StringUtil::format(
        "<tr><td><em>Replication Thread #$0:</em> $1</td></tr>",
        i + 1,
        ctx->replication_worker->getReplicationInfo(i)->toString());
  }
  html += "</table>";

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
  //html += kMainMenu;

  html += StringUtil::format(
      "<h2>Namespace: &nbsp; <p style='font-weight:normal'>$0</p></h2>",
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
  auto ctx = db_->getSession()->getDatabaseContext();
  auto pmap = ctx->partition_map;

  String html;
  html += kStyleSheet;
  //html += kMainMenu;

  html += StringUtil::format(
      "<h2>Table: &nbsp; <p style='font-weight:normal'>"
      "<a href='/zstatus/db/$0'>$0</a> &mdash;"
      "$1</p></h2>",
      db_namespace,
      table_name);

  auto table = pmap->findTable(
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
      "<p><em>Metatdata TXNID:</em> $0 [$1]</p> &mdash; ",
      SHA1Hash(
          table_cfg.metadata_txnid().data(),
          table_cfg.metadata_txnid().size()).toString(),
      table_cfg.metadata_txnseq());

  MetadataClient metadata_client(ctx->config_directory);
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
          memcpy((char*) &keyrange_uint, e.begin.data(), sizeof(uint64_t));
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
  auto ctx = db_->getSession()->getDatabaseContext();
  auto pmap = ctx->partition_map;
  String html;
  html += kStyleSheet;
  //html += kMainMenu;

  html += StringUtil::format(
      "<h2>Partition: &nbsp; <p style='font-weight:normal'>"
      "<a href='/zstatus/db/$0'>$0</a> &mdash;"
      "<a href='/zstatus/db/$0/$1'>$1</a> &mdash;"
      "$2</p></h2>",
      db_namespace,
      table_name,
      partition_key.toString());

  auto partition = pmap->findPartition(
      db_namespace,
      table_name,
      partition_key);

  if (partition.isEmpty()) {
    html += "ERROR: PARTITION NOT FOUND!";
  } else {
    auto table = partition.get()->getTable();
    auto snap = partition.get()->getSnapshot();
    auto state = snap->state;
    auto repl = partition.get()->getReplicationStrategy();

    if (table->partitionerType() == TBL_PARTITION_TIMEWINDOW &&
        state.partition_keyrange_begin().size() == 8 &&
        state.partition_keyrange_end().size() == 8) {
      uint64_t ts_begin;
      uint64_t ts_end;
      memcpy((char*) &ts_begin, state.partition_keyrange_begin().data(), 8);
      memcpy((char*) &ts_end, state.partition_keyrange_end().data(), 8);

      html += StringUtil::format(
          "<p><em>Keyrange:</em> $0 [$1] - $2 [$3]</p> &mdash; ",
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
          "<p><em>Keyrange:</em> $0 - $1</p> &mdash; ",
          range_begin,
          range_end);
    }

    if (table->partitionerType() == TBL_PARTITION_STRING) {
      html += StringUtil::format(
          "<p><em>Keyrange:</em> $0 - $1</p> &mdash; ",
          state.partition_keyrange_begin(),
          state.partition_keyrange_end());
    }

    html += StringUtil::format(
        "<p><em>Lifecycle State</em>: $0</p> &mdash; ",
        PartitionLifecycleState_Name(snap->state.lifecycle_state()));

    html += StringUtil::format(
        "<p><em>Needs Replication?</em>: $0</p> &mdash; ",
        repl->needsReplication());

    html += StringUtil::format(
        "<p><em>Splitting?</em>: $0</p> &mdash; ",
        partition.get()->isSplitting());

    html += StringUtil::format(
        "<p><em>Joining Servers?</em>: $0</p> &mdash; ",
        snap->state.has_joining_servers());

    html += StringUtil::format(
        "<p><em>Size (Disk)</em>: $0MB</p> &mdash; ",
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
