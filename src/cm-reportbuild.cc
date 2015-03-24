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
#include "reports/JoinedQueryTableReport.h"
#include "reports/CTRByPositionReport.h"
#include "reports/CTRCounterSSTableSink.h"

using namespace fnord;
using namespace cm;

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

  Set<uint64_t> generations { };

  /* find all generations */
  auto dir = flags.getString("artifacts");
  FileUtil::ls(dir, [&generations] (const String& file) -> bool {
    String prefix = "dawanda_joined_queries.";
    if (StringUtil::beginsWith(file, prefix)) {
      generations.emplace(std::stoul(file.substr(prefix.length())));
    }
    return true;
  });

  for (const auto& g : generations) {
    auto jq_report = new JoinedQueryTableReport(Set<String> {
        StringUtil::format("$0/dawanda_joined_queries.$1.sstable", dir, g) });
    report_builder.addReport(jq_report);

    auto ctr_by_posi_report = new CTRByPositionReport(ItemEligibility::ALL);
    ctr_by_posi_report->addReport(new CTRCounterSSTableSink(
        StringUtil::format("$0/dawanda_ctr_by_position.$1.sstable", dir, g)));
    jq_report->addReport(ctr_by_posi_report);
  }

  report_builder.buildAll();
  return 0;
}

