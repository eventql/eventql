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
#include "common.h"
#include "CustomerNamespace.h"
#include "FeatureSchema.h"
#include "JoinedQuery.h"
#include "CTRCounter.h"
#include "IndexReader.h"
#include "analytics/ReportBuilder.h"
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


Set<uint64_t> mkGenerations(
    uint64_t window_secs,
    uint64_t range_secs,
    uint64_t now_secs = 0) {
  Set<uint64_t> generations;
  auto now = now_secs == 0 ? WallClock::unixMicros() : now_secs * kMicrosPerSecond;
  auto gen_window = kMicrosPerSecond * window_secs;
  for (uint64_t i = 1; i < kMicrosPerSecond * range_secs; i += gen_window) {
    generations.emplace((now - i) / gen_window);
  }

  return generations;
}

int main(int argc, const char** argv) {
  fnord::Application::init();
  fnord::Application::logToStderr();

  fnord::cli::FlagParser flags;

  flags.defineFlag(
      "conf",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      "./conf",
      "conf directory",
      "<path>");

  flags.defineFlag(
      "index",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "index directory",
      "<path>");

  flags.defineFlag(
      "artifacts",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "artifact directory",
      "<path>");

  flags.defineFlag(
      "loop",
      fnord::cli::FlagParser::T_SWITCH,
      false,
      NULL,
      NULL,
      "loop",
      "<switch>");

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
  auto conf_path = flags.getString("conf");
  auto dir = flags.getString("artifacts");

  /* open index */
  auto index_reader = cm::IndexReader::openIndex(index_path);
  auto analyzer = RefPtr<fts::Analyzer>(new fts::Analyzer(conf_path));

  /* set up reportbuilder */
  cm::ReportBuilder report_builder;

  /* 4 hourly reports */
  for (const auto& g : mkGenerations(
        4 * kSecondsPerHour,
        60 * kSecondsPerDay)) {
    /* dawanda: map joined queries */
    auto jq_source = new JoinedQueryTableSource(
        StringUtil::format("$0/dawanda_joined_queries.$1.sstable", dir, g));

    report_builder.addReport(
        new CTRBySearchTermCrossCategoryMapper(
            jq_source,
            new CTRCounterTableSink(
                g * kMicrosPerHour * 4,
                (g + 1) * kMicrosPerHour * 4,
                StringUtil::format(
                    "$0/dawanda_ctr_by_searchterm_cross_e1.$1.sstable",
                    dir,
                    g)),
            "category1",
            ItemEligibility::ALL,
            analyzer,
            index_reader));

    report_builder.addReport(
        new CTRBySearchTermCrossCategoryMapper(
            jq_source,
            new CTRCounterTableSink(
                g * kMicrosPerHour * 4,
                (g + 1) * kMicrosPerHour * 4,
                StringUtil::format(
                    "$0/dawanda_ctr_by_searchterm_cross_e2.$1.sstable",
                    dir,
                    g)),
            "category2",
            ItemEligibility::ALL,
            analyzer,
            index_reader));

    report_builder.addReport(
        new CTRBySearchTermCrossCategoryMapper(
            jq_source,
            new CTRCounterTableSink(
                g * kMicrosPerHour * 4,
                (g + 1) * kMicrosPerHour * 4,
                StringUtil::format(
                    "$0/dawanda_ctr_by_searchterm_cross_e3.$1.sstable",
                    dir,
                    g)),
            "category3",
            ItemEligibility::ALL,
            analyzer,
            index_reader));
  }

  /* daily reports */
  for (const auto& og : mkGenerations(
        1 * kSecondsPerDay,
        60 * kSecondsPerDay)) {
    auto day_gens = mkGenerations(
        4 * kSecondsPerHour,
        1 * kSecondsPerDay,
        og * kSecondsPerDay);

    /* dawanda: related search terms */
    Set<String> month_sources;
    for (const auto& ig : mkGenerations(
        4 * kSecondsPerHour,
        30 * kSecondsPerDay,
        og * kSecondsPerDay)) {
      month_sources.emplace(StringUtil::format(
          "$0/dawanda_joined_sessions.$1.sstable",
          dir,
          ig));
    }

    //report_builder.addReport(
    //    new RelatedTermsMapper(
    //        new CTRCounterTableSource(related_terms_sources),
    //        new TermInfoTableSink(
    //            StringUtil::format(
    //                "$0/dawanda_related_terms_30d.$1.sstable",
    //                dir,
    //                og))));

    /* dawanda: search term x e{1,2,3} id */
    Set<String> term_cross_e1_sources;
    for (const auto& ig : day_gens) {
      term_cross_e1_sources.emplace(StringUtil::format(
          "$0/dawanda_ctr_by_searchterm_cross_e1.$1.sstable",
          dir,
          ig));
    }

    Set<String> term_cross_e1_rollup_sources;
    for (const auto& ig : mkGenerations(
        1 * kSecondsPerDay,
        30 * kSecondsPerDay,
        og * kSecondsPerDay)) {
      term_cross_e1_rollup_sources.emplace(StringUtil::format(
          "$0/dawanda_top_cats_by_searchterm_e1.$1.sstable",
          dir,
          ig));
    }

    report_builder.addReport(
        new TopCategoriesByTermMapper(
            new CTRCounterTableSource(term_cross_e1_sources),
            new TermInfoTableSink(
                StringUtil::format(
                    "$0/dawanda_top_cats_by_searchterm_e1.$1.sstable",
                    dir,
                    og)),
            "e1-"));


    report_builder.addReport(
        new TermInfoMergeReducer(
            new TermInfoTableSource(term_cross_e1_rollup_sources),
            new TermInfoTableSink(
                StringUtil::format(
                    "$0/dawanda_top_cats_by_searchterm_e1_30d.$1.sstable",
                    dir,
                    og))));


    /* merge all term info 30d reports */
    report_builder.addReport(
        new TermInfoMergeReducer(
            new TermInfoTableSource(Set<String> {
              StringUtil::format(
                    "$0/dawanda_related_terms_30d.$1.sstable",
                    dir,
                    og),
              StringUtil::format(
                    "$0/dawanda_top_cats_by_searchterm_e1_30d.$1.sstable",
                    dir,
                    og)
            }),
            new TermInfoTableSink(
                StringUtil::format(
                    "$0/dawanda_termstats_30d.$1.sstable",
                    dir,
                    og))));
  }

  if (flags.isSet("loop")) {
    report_builder.buildLoop();
  } else {
    report_builder.buildAll();
  }
  return 0;
}

