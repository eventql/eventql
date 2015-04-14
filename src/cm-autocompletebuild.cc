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
#include "fnord-base/cli/flagparser.h"
#include "fnord-base/util/SimpleRateLimit.h"
#include "fnord-base/InternMap.h"
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
#include "fnord-eventdb/TableReader.h"
#include "common.h"
#include "schemas.h"
#include "CustomerNamespace.h"
#include "FeatureSchema.h"
#include "JoinedQuery.h"
#include "CTRCounter.h"
#include "IndexReader.h"
#include "analytics/ReportBuilder.h"
#include "analytics/AnalyticsTableScanSource.h"
#include "analytics/JoinedQueryTableSource.h"
#include "analytics/CTRByPositionMapper.h"
#include "analytics/CTRByPageMapper.h"
#include "analytics/CTRBySearchQueryMapper.h"
#include "analytics/CTRByQueryAttributeMapper.h"
#include "analytics/CTRBySearchTermCrossCategoryMapper.h"
#include "analytics/CTRStatsMapper.h"
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

  //flags.defineFlag(
  //    "conf",
  //    cli::FlagParser::T_STRING,
  //    false,
  //    NULL,
  //    "./conf",
  //    "conf directory",
  //    "<path>");

  flags.defineFlag(
      "index",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "index directory",
      "<path>");

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

  auto index_path = flags.getString("index");
  //auto conf_path = flags.getString("conf");
  auto tempdir = flags.getString("tempdir");
  auto datadir = flags.getString("datadir");

  /* open index */
  auto index_reader = cm::IndexReader::openIndex(index_path);
  //auto analyzer = RefPtr<fts::Analyzer>(new fts::Analyzer(conf_path));

  /* set up reportbuilder */
  cm::ReportBuilder report_builder;

  auto buildid = 0;

  auto table = eventdb::TableReader::open(
      "dawanda_joined_sessions",
      flags.getString("replica"),
      flags.getString("datadir"),
      joinedSessionsSchema());

  auto snap = table->getSnapshot();

  //Vector<String> input_tables;
  Vector<String> searchterm_x_e1_tables;

  for (const auto& c : snap->head->chunks) {
    auto input_table = StringUtil::format(
        "$0/$1.$2.$3.cst",
        datadir,
        "dawanda_joined_sessions",
        c.replica_id,
        c.chunk_id);

    /* map serchterm x e1 */
    auto searchterm_x_e1_table = StringUtil::format(
        "$0/dawanda_ctr_by_searchterm_cross_e1.$1.$2.sst",
        tempdir,
        c.replica_id,
        c.chunk_id);

    searchterm_x_e1_tables.emplace_back(searchterm_x_e1_table);
    report_builder.addReport(
        new CTRBySearchTermCrossCategoryMapper(
            new AnalyticsTableScanSource(input_table),
            new CTRCounterTableSink(0, 0, searchterm_x_e1_table),
            "category1",
            index_reader));
  }

//  report_builder.addReport(
//      new RelatedTermsMapper(
//          new CTRCounterTableSource(related_terms_sources),
//          new TermInfoTableSink(
//              StringUtil::format(
//                  "$0/dawanda_related_terms_30d.$1.sstable",
//                  dir,
//                  og))));
//
  report_builder.addReport(
      new TermInfoMergeReducer(
          new TermInfoTableSource(Set<String> {
            StringUtil::format(
                  "$0/dawanda_related_terms.$1.sstable",
                  tempdir,
                  buildid),
            StringUtil::format(
                  "$0/dawanda_top_cats_by_searchterm_e1.$1.sstable",
                  tempdir,
                  buildid)
          }),
          new TermInfoTableSink(
              StringUtil::format(
                  "$0/dawanda_termstats.$1.sstable",
                  tempdir,
                  buildid))));


  report_builder.buildAll();
  return 0;
}

