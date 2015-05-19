/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <algorithm>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "fnord-base/io/fileutil.h"
#include "fnord-base/thread/eventloop.h"
#include "fnord-base/application.h"
#include "fnord-base/logging.h"
#include "fnord-base/Language.h"
#include "fnord-base/random.h"
#include "fnord-base/fnv.h"
#include "fnord-base/cli/flagparser.h"
#include "fnord-base/util/SimpleRateLimit.h"
#include "fnord-base/InternMap.h"
#include "fnord-base/thread/threadpool.h"
#include "fnord-dproc/Application.h"
#include "fnord-dproc/LocalScheduler.h"
#include "fnord-json/json.h"
#include "fnord-mdb/MDB.h"
#include "fnord-mdb/MDBUtil.h"
#include "fnord-sstable/sstablereader.h"
#include "fnord-sstable/sstablewriter.h"
#include "fnord-sstable/SSTableColumnSchema.h"
#include "fnord-sstable/SSTableColumnReader.h"
#include "fnord-sstable/SSTableColumnWriter.h"
#include "fnord-http/httpconnectionpool.h"
#include <fnord-fts/fts.h>
#include <fnord-fts/fts_common.h>
#include "fnord-logtable/TableReader.h"
#include "common.h"
#include "schemas.h"
#include "CustomerNamespace.h"
#include "CTRCounter.h"
#include "analytics/ReportBuilder.h"
#include "analytics/AnalyticsTableScanSource.h"
#include "analytics/CTRByShopMapper.h"
#include "analytics/ECommerceStatsByShopMapper.h"
#include "analytics/ProductStatsByShopMapper.h"
#include "analytics/CTRCounterMergeReducer.h"
#include "AnalyticsTableScanParams.pb.h"

using namespace fnord;
using namespace cm;

fnord::thread::EventLoop ev;

int main(int argc, const char** argv) {
  fnord::Application::init();
  fnord::Application::logToStderr();

  fnord::cli::FlagParser flags;

  flags.defineFlag(
      "tempdir",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "artifact directory",
      "<path>");

  flags.defineFlag(
      "loglevel",
      fnord::cli::FlagParser::T_STRING,
      false,
      NULL,
      "INFO",
      "loglevel",
      "<level>");

  flags.parseArgv(argc, argv);

  Logger::get()->setMinimumLogLevel(
      strToLogLevel(flags.getString("loglevel")));

  /* start event loop */
  auto evloop_thread = std::thread([] {
    ev.run();
  });

  //auto index_path = flags.getString("index");
  //auto conf_path = flags.getString("conf");
  auto tempdir = flags.getString("tempdir");

  /* open index */
  //auto index_reader = cm::IndexReader::openIndex(index_path);
  //auto analyzer = RefPtr<fts::Analyzer>(new fts::Analyzer(conf_path));

  /* set up reportbuilder */
  thread::ThreadPool tpool;
  http::HTTPConnectionPool http(&ev);
  tsdb::TSDBClient tsdb("http://nue03.prod.fnrd.net:7003/tsdb", &http);

  Random rnd;
  auto buildid = rnd.hex128();
  /*
  cm::ReportBuilder report_builder(&tpool);

  Set<String> input_tables;
  Set<String> input_table_files;

  {
    URI uri(StringUtil::format(
        "http://nue03.prod.fnrd.net:7003/tsdb/list_chunks?stream=$0&from=$1&until=$2",
        "joined_sessions.dawanda",
        WallClock::unixMicros() - 90 * kMicrosPerDay,
        WallClock::unixMicros() - 6 * kMicrosPerHour));

    http::HTTPRequest req(http::HTTPMessage::M_GET, uri.pathAndQuery());
    req.addHeader("Host", uri.hostAndPort());
    req.addHeader("Content-Type", "application/fnord-msg");

    auto res = http.executeRequest(req);
    res.wait();

    const auto& r = res.get();
    if (r.statusCode() != 200) {
      RAISEF(kRuntimeError, "received non-200 response: $0", r.body().toString());
    }

    const auto& body = r.body();
    util::BinaryMessageReader reader(body.data(), body.size());
    while (reader.remaining() > 0) {
      input_tables.emplace(reader.readLenencString());
    }
  }

  for (const auto& tbl : input_tables) {
    URI uri(StringUtil::format(
        "http://nue03.prod.fnrd.net:7003/tsdb/fetch_derived_dataset?chunk=$0&derived_dataset=cstable",
        tbl));

    http::HTTPRequest req(http::HTTPMessage::M_GET, uri.pathAndQuery());
    req.addHeader("Host", uri.hostAndPort());
    req.addHeader("Content-Type", "application/fnord-msg");

    auto res = http.executeRequest(req);
    res.wait();

    const auto& r = res.get();
    if (r.statusCode() != 200) {
      RAISEF(kRuntimeError, "received non-200 response: $0", r.body().toString());
    }

    fnord::iputs("src tbl: $0", r.body().toString());
    input_table_files.emplace(r.body().toString());
  }

  for (const auto& input_table : input_table_files) {
    FNV<uint64_t> fnv;
    auto h = fnv.hash(input_table);

  }

  for (const auto& input_table : input_table_files) {
    FNV<uint64_t> fnv;
    auto h = fnv.hash(input_table);

    auto table = StringUtil::format(
        "$0/shopstats-ecommerce-dawanda.$1.sst",
        tempdir,
        StringUtil::hexPrint(&h, sizeof(h), false));

    tables.emplace(table);
    report_builder.addReport(
        new ECommerceStatsByShopMapper(
            new AnalyticsTableScanSource(input_table),
            new ShopStatsTableSink(table)));
  }

  for (const auto& input_table : input_table_files) {
    FNV<uint64_t> fnv;
    auto h = fnv.hash(input_table);

    auto table = StringUtil::format(
        "$0/shopstats-products-dawanda.$1.sst",
        tempdir,
        StringUtil::hexPrint(&h, sizeof(h), false));

    tables.emplace(table);
    report_builder.addReport(
        new ProductStatsByShopMapper(
            new AnalyticsTableScanSource(input_table),
            new ShopStatsTableSink(table)));
  }

  */

  dproc::Application app("cm.shopstats");

  app.registerTaskFactory(
      "CTRByShopMapper",
      [&tsdb] (const Buffer& p) -> RefPtr<dproc::Task> {
        auto params = msg::decode<AnalyticsTableScanMapperParams>(p);

        return new CTRByShopMapper(
            new AnalyticsTableScanSource(params, &tsdb),
            new ShopStatsTableSink("fnord"));
      });

  app.registerTaskFactory(
      "ShopStatsReducer",
      [&tsdb] (const Buffer& p) -> RefPtr<dproc::Task> {
        auto reducer_params = msg::decode<AnalyticsTableScanReducerParams>(p);

        auto stream = "joined_sessions." + reducer_params.customer();
        auto partitions = tsdb.listPartitions(
            stream,
            reducer_params.from_unixmicros(),
            reducer_params.until_unixmicros());

        List<dproc::TaskDependency> map_chunks;
        for (const auto& part : partitions) {
          AnalyticsTableScanMapperParams map_chunk_params;
          map_chunk_params.set_stream_key(stream);
          map_chunk_params.set_partition_key(part);

          map_chunks.emplace_back(dproc::TaskDependency {
            .task_name = "CTRByShopMapper",
            .params = *msg::encode(map_chunk_params)
          });
        }

        return new ShopStatsMergeReducer(
            new ShopStatsTableSource(map_chunks),
            new ShopStatsTableSink("fnord"));
      });


  dproc::LocalScheduler sched(flags.getString("tempdir"));
  sched.start();
  //report_builder.buildAll();

  fnord::logInfo(
      "cm.reportbuild",
      "Build completed: shopstats-full-dawanda.$0.sstable",
      buildid);

  AnalyticsTableScanReducerParams params;
  params.set_customer("dawanda");
  params.set_from_unixmicros(WallClock::unixMicros() - 7 * kMicrosPerDay);
  params.set_until_unixmicros(WallClock::unixMicros() - 6 * kMicrosPerHour);

  auto res = sched.run(&app, "ShopStatsReducer", *msg::encode(params));

  sched.stop();
  ev.shutdown();
  evloop_thread.join();

  exit(0);
}

