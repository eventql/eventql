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
#include "analytics/CTRCounterMergeReducer.h"
#include "analytics/CTRCounterTableSink.h"
#include "analytics/CTRCounterTableSource.h"
#include "analytics/RelatedTermsMapper.h"
#include "analytics/TopCategoriesByTermMapper.h"
#include "analytics/TermInfoMergeReducer.h"

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
  cm::ReportBuilder report_builder(&tpool);

  Random rnd;
  auto buildid = rnd.hex128();

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
        "http://nue03.prod.fnrd.net:7003/tsdb/list_files?chunk=$0",
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

    const auto& body = r.body();
    util::BinaryMessageReader reader(body.data(), body.size());
    while (reader.remaining() > 0) {
      input_table_files.emplace(reader.readLenencString());
    }
  }

  Set<String> tables;
  for (const auto& input_table : input_table_files) {
    FNV<uint64_t> fnv;
    auto h = fnv.hash(input_table);

    auto table = StringUtil::format(
        "$0/shopstats-ctr-dawanda.$1.sst",
        tempdir,
        StringUtil::hexPrint(&h, sizeof(h), false));

    tables.emplace(table);
    report_builder.addReport(
        new CTRByShopMapper(
            new AnalyticsTableScanSource(input_table),
            new ShopStatsTableSink(table)));
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

  report_builder.addReport(
      new ShopStatsMergeReducer(
          new ShopStatsTableSource(tables),
          new ShopStatsTableSink(
              StringUtil::format(
                  "$0/shopstats-full-dawanda.$1.sstable",
                  tempdir,
                  buildid))));

  report_builder.buildAll();

  fnord::logInfo(
      "cm.reportbuild",
      "Build completed: shopstats-full-dawanda.$0.sstable",
      buildid);

  ev.shutdown();
  evloop_thread.join();

  return 0;
}

