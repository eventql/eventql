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

using namespace fnord;

typedef Tuple<String, uint64_t, uint64_t> OutputRow;
typedef HashMap<void*, cm::CTRCounterData> CounterMap;

InternMap intern_map;

void indexJoinedQuery(
    const cm::JoinedQuery& query,
    const String& feature_name,
    cm::ItemEligibility eligibility,
    CounterMap* counters) {
  if (!isQueryEligible(eligibility, query)) {
    return;
  }

  auto fstr_opt = cm::extractAttr(query.attrs, feature_name);
  if (fstr_opt.isEmpty()) {
    return;
  }

  auto fstr = URI::urlDecode(fstr_opt.get());

/*
  switch (feature_prep_) {
    case FeaturePrep::NONE:
      break;

    case FeaturePrep::BAGOFWORDS_DE: {
      Set<String> tokens;
      cm::tokenizeAndStem(
          cm::Language::GERMAN,
          fstr,
          &tokens);

      fstr = cm::joinBagOfWords(tokens);
      break;
    }
  }
*/

  auto& counter = (*counters)[intern_map.internString(fstr)];
  auto& global_counter = (*counters)[nullptr];
  counter.num_views++;
  global_counter.num_views++;
  bool any_clicked = false;

  for (const auto& item : query.items) {
    if (item.clicked) {
      if (!any_clicked) {
        any_clicked = true;
        counter.num_clicked++;
        global_counter.num_clicked++;
      }

      counter.num_clicks++;
      global_counter.num_clicks++;
    }
  }
}

/* write output table */
void writeOutputTable(
    const String& filename,
    const CounterMap& counters,
    uint64_t start_time,
    uint64_t end_time) {
  /* prepare output sstable schema */
  sstable::SSTableColumnSchema sstable_schema;
  sstable_schema.addColumn("num_views", 1, sstable::SSTableColumnType::UINT64);
  sstable_schema.addColumn("num_clicks", 2, sstable::SSTableColumnType::UINT64);
  sstable_schema.addColumn("num_clicked", 3, sstable::SSTableColumnType::UINT64);

  HashMap<String, String> out_hdr;
  out_hdr["start_time"] = StringUtil::toString(start_time);
  out_hdr["end_time"] = StringUtil::toString(end_time);
  auto outhdr_json = json::toJSONString(out_hdr);

  /* open output sstable */
  fnord::logInfo("cm.ctrstats", "Writing results to: $0", filename);
  auto sstable_writer = sstable::SSTableWriter::create(
      filename,
      sstable::IndexProvider{},
      outhdr_json.data(),
      outhdr_json.length());

  for (const auto& p : counters) {
    sstable::SSTableColumnWriter cols(&sstable_schema);
    cols.addUInt64Column(1, p.second.num_views);
    cols.addUInt64Column(2, p.second.num_clicks);
    cols.addUInt64Column(3, p.second.num_clicked);

    sstable_writer->appendRow(
        p.first == nullptr ? "__GLOBAL" : intern_map.getString(p.first),
        cols);
  }

  sstable_schema.writeIndex(sstable_writer.get());
  sstable_writer->finalize();
}

int main(int argc, const char** argv) {
  fnord::Application::init();
  fnord::Application::logToStderr();

  fnord::cli::FlagParser flags;

  flags.defineFlag(
      "output_file",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "output file path",
      "<path>");

  flags.defineFlag(
      "query_feature",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "query feature",
      "<feature>");

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

  CounterMap counters;
  auto query_feature = flags.getString("query_feature");
  auto start_time = std::numeric_limits<uint64_t>::max();
  auto end_time = std::numeric_limits<uint64_t>::min();

  /* read input tables */
  auto sstables = flags.getArgv();
  for (int tbl_idx = 0; tbl_idx < sstables.size(); ++tbl_idx) {
    const auto& sstable = sstables[tbl_idx];
    fnord::logInfo("cm.ctrstats", "Importing sstable: $0", sstable);

    /* read sstable header */
    sstable::SSTableReader reader(File::openFile(sstable, File::O_READ));

    if (reader.bodySize() == 0) {
      fnord::logCritical("cm.ctrstats", "unfinished sstable: $0", sstable);
      exit(1);
    }

    /* read report header */
    auto hdr = json::parseJSON(reader.readHeader());

    auto tbl_start_time = json::JSONUtil::objectGetUInt64(
        hdr.begin(),
        hdr.end(),
        "start_time").get();

    auto tbl_end_time = json::JSONUtil::objectGetUInt64(
        hdr.begin(),
        hdr.end(),
        "end_time").get();

    if (tbl_start_time < start_time) {
      start_time = tbl_start_time;
    }

    if (tbl_end_time > end_time) {
      end_time = tbl_end_time;
    }

    /* get sstable cursor */
    auto cursor = reader.getCursor();
    auto body_size = reader.bodySize();
    int row_idx = 0;

    /* status line */
    util::SimpleRateLimitedFn status_line(kMicrosPerSecond, [&] () {
      fnord::logInfo(
          "cm.ctrstats",
          "[$1/$2] [$0%] Reading sstable... rows=$3",
          (size_t) ((cursor->position() / (double) body_size) * 100),
          tbl_idx + 1, sstables.size(), row_idx);
    });

    /* read sstable rows */
    for (; cursor->valid(); ++row_idx) {
      status_line.runMaybe();

      auto val = cursor->getDataBuffer();
      Option<cm::JoinedQuery> q;

      try {
        q = Some(json::fromJSON<cm::JoinedQuery>(val));
      } catch (const Exception& e) {
        //fnord::logWarning("cm.ctrstats", e, "invalid json: $0", val.toString());
      }

      if (!q.isEmpty()) {
        indexJoinedQuery(
            q.get(),
            query_feature,
            cm::ItemEligibility::DAWANDA_ALL_NOBOTS,
            &counters);
      }

      if (!cursor->next()) {
        break;
      }
    }

    status_line.runForce();
  }

  /* write output table */
  writeOutputTable(
      flags.getString("output_file"),
      counters,
      start_time,
      end_time);

  return 0;
}

