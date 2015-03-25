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
#include "common.h"
#include "CustomerNamespace.h"
#include "FeatureSchema.h"
#include "JoinedQuery.h"
#include "CTRCounter.h"
#include <fnord-fts/fts.h>
#include <fnord-fts/fts_common.h>
#include "reports/ReportBuilder.h"
#include "reports/JoinedQueryTableSource.h"
#include "reports/CTRByPositionReport.h"
#include "reports/CTRReport.h"
#include "reports/CTRCounterMerge.h"
#include "reports/CTRCounterTableSink.h"
#include "reports/CTRCounterTableSource.h"

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

  fnord::fts::Analyzer analyzer(flags.getString("conf"));
  cm::ReportBuilder report_builder;

  auto dir = flags.getString("artifacts");

  /* 4 hourly reports */
  for (const auto& g : mkGenerations(
        4 * kSecondsPerHour,
        60 * kSecondsPerDay)) {
    /* dawanda: map joined queries */
    auto jq_source = new JoinedQueryTableSource(
        StringUtil::format("$0/dawanda_joined_queries.$1.sstable", dir, g));

    report_builder.addReport(
        new CTRByPositionReport(
            jq_source,
            new CTRCounterTableSink(
                StringUtil::format(
                    "$0/dawanda_ctr_by_position.$1.sstable",
                    dir,
                    g)),
            ItemEligibility::ALL));

    report_builder.addReport(
        new CTRReport(
            jq_source,
            new CTRCounterTableSink(
                StringUtil::format(
                    "$0/dawanda_ctr_stats.$1.sstable",
                    dir,
                    g)),
            ItemEligibility::ALL));
  }

  /* daily reports */
  for (const auto& og : mkGenerations(
        1 * kSecondsPerDay,
        60 * kSecondsPerDay)) {
    auto day_gens = mkGenerations(
        4 * kSecondsPerHour,
        1 * kSecondsPerDay,
        og * kSecondsPerDay);

    /* dawanda: roll up ctr stats */
    Set<String> ctr_stats_sources;
    for (const auto& ig : day_gens) {
      ctr_stats_sources.emplace(
          StringUtil::format("$0/dawanda_ctr_stats.$1.sstable", dir, ig));
    }

    report_builder.addReport(
        new CTRCounterMerge(
            new CTRCounterTableSource(ctr_stats_sources),
            new CTRCounterTableSink(
                StringUtil::format(
                    "$0/dawanda_ctr_stats_daily.$1.sstable",
                    dir,
                    og))));

    /* dawanda: roll up ctr positions */
    Set<String> ctr_posi_sources;
    for (const auto& ig : day_gens) {
      ctr_posi_sources.emplace(
          StringUtil::format("$0/dawanda_ctr_by_position.$1.sstable", dir, ig));
    }

    report_builder.addReport(
        new CTRCounterMerge(
            new CTRCounterTableSource(ctr_posi_sources),
            new CTRCounterTableSink(
                StringUtil::format(
                    "$0/dawanda_ctr_by_position_daily.$1.sstable",
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

