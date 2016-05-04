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
#include "stx/io/fileutil.h"
#include "stx/application.h"
#include "stx/logging.h"
#include "stx/Language.h"
#include "stx/cli/flagparser.h"
#include "stx/util/SimpleRateLimit.h"
#include "stx/InternMap.h"
#include "stx/json/json.h"
#include "eventql/util/mdb/MDB.h"
#include "eventql/util/mdb/MDBUtil.h"
#include "sstable/sstablereader.h"
#include "sstable/SSTableEditor.h"
#include "sstable/SSTableColumnSchema.h"
#include "sstable/SSTableColumnReader.h"
#include "sstable/SSTableColumnWriter.h"
#include <eventql/util/fts.h>
#include <eventql/util/fts_common.h>
#include "common.h"
#include "CustomerNamespace.h"

#
#include "eventql/CTRCounter.h"
#include "IndexReader.h"
#include "eventql/ReportBuilder.h"
#include "eventql/JoinedQueryTableSource.h"
#include "eventql/CTRByPositionMapper.h"
#include "eventql/CTRByPageMapper.h"
#include "eventql/CTRBySearchQueryMapper.h"
#include "eventql/CTRByQueryAttributeMapper.h"
#include "eventql/CTRBySearchTermCrossCategoryMapper.h"
#include "eventql/CTRStatsMapper.h"
#include "eventql/CTRCounterMergeReducer.h"
#include "eventql/CTRCounterTableSink.h"
#include "eventql/CTRCounterTableSource.h"
#include "eventql/RelatedTermsMapper.h"
#include "eventql/TopCategoriesByTermMapper.h"
#include "eventql/TermInfoMergeReducer.h"

using namespace stx;
using namespace zbase;


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
  stx::Application::init();
  stx::Application::logToStderr();

  stx::cli::FlagParser flags;

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
      stx::cli::FlagParser::T_SWITCH,
      false,
      NULL,
      NULL,
      "loop",
      "<switch>");

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

  auto index_path = flags.getString("index");
  auto conf_path = flags.getString("conf");
  auto dir = flags.getString("artifacts");

  /* open index */
  auto index_reader = zbase::IndexReader::openIndex(index_path);
  auto analyzer = RefPtr<fts::Analyzer>(new fts::Analyzer(conf_path));

  /* set up reportbuilder */
  zbase::ReportBuilder report_builder;

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

