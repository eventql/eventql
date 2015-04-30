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
#include "fnord-base/application.h"
#include "fnord-base/logging.h"
#include "fnord-base/Language.h"
#include "fnord-base/random.h"
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
#include "analytics/CTRCounterMergeReducer.h"
#include "analytics/CTRCounterTableSink.h"
#include "analytics/CTRCounterTableSource.h"
#include "analytics/RelatedTermsMapper.h"
#include "analytics/TopCategoriesByTermMapper.h"
#include "analytics/TermInfoMergeReducer.h"

using namespace fnord;
using namespace cm;

int main(int argc, const char** argv) {
  fnord::Application::init();
  fnord::Application::logToStderr();

  fnord::cli::FlagParser flags;

  flags.defineFlag(
      "replica",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "replica id",
      "<id>");

  flags.defineFlag(
      "datadir",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "artifact directory",
      "<path>");

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

  //auto index_path = flags.getString("index");
  //auto conf_path = flags.getString("conf");
  auto tempdir = flags.getString("tempdir");
  auto datadir = flags.getString("datadir");

  /* open index */
  //auto index_reader = cm::IndexReader::openIndex(index_path);
  //auto analyzer = RefPtr<fts::Analyzer>(new fts::Analyzer(conf_path));

  /* set up reportbuilder */
  thread::ThreadPool tpool;
  cm::ReportBuilder report_builder(&tpool);

  Random rnd;
  auto buildid = rnd.hex128();

  auto table = logtable::TableReader::open(
      "joined_sessions-dawanda",
      flags.getString("replica"),
      flags.getString("datadir"),
      joinedSessionsSchema());

  auto snap = table->getSnapshot();

  Set<String> tables;

  for (const auto& c : snap->head->chunks) {
    auto input_table = StringUtil::format(
        "$0/$1.$2.$3.cst",
        datadir,
        "joined_sessions-dawanda",
        c.replica_id,
        c.chunk_id);

    auto table = StringUtil::format(
        "$0/shopstats-ctr-dawanda.$1.$2.sst",
        tempdir,
        c.replica_id,
        c.chunk_id);

    tables.emplace(table);
    report_builder.addReport(
        new CTRByShopMapper(
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

  return 0;
}

