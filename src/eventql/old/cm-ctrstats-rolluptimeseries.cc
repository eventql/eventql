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
#include "eventql/util/io/fileutil.h"
#include "eventql/util/application.h"
#include "eventql/util/logging.h"
#include "eventql/util/cli/flagparser.h"
#include "eventql/util/util/SimpleRateLimit.h"
#include "eventql/util/mdb/MDB.h"
#include "eventql/util/mdb/MDBUtil.h"
#include "eventql/util/json/json.h"
#include "eventql/infra/sstable/sstablereader.h"
#include "eventql/infra/sstable/SSTableEditor.h"
#include "eventql/infra/sstable/SSTableColumnSchema.h"
#include "eventql/infra/sstable/SSTableColumnReader.h"
#include "eventql/infra/sstable/SSTableColumnWriter.h"
#include "common.h"
#include "CustomerNamespace.h"

#
#include "eventql/CTRCounter.h"

using namespace stx;

typedef Tuple<String, uint64_t, double> OutputRow;
typedef HashMap<String, HashMap<uint64_t, zbase::CTRCounterData>> CounterMap;

/* read all input sstables */
void importInputTables(
    const Vector<String> sstables,
    Duration aggr_period,
    CounterMap* counters) {
  for (int tbl_idx = 0; tbl_idx < sstables.size(); ++tbl_idx) {
    const auto& sstable = sstables[tbl_idx];
    stx::logInfo("cm.ctrstats", "Importing sstable: $0", sstable);

    /* read sstable heade r*/
    sstable::SSTableReader reader(File::openFile(sstable, File::O_READ));
    sstable::SSTableColumnSchema schema;
    schema.loadIndex(&reader);

    if (reader.bodySize() == 0) {
      stx::logCritical("cm.ctrstats", "unfinished sstable: $0", sstable);
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

    auto tbl_time = tbl_start_time + (tbl_end_time - tbl_start_time) / 2;
    auto tbl_window = tbl_time / aggr_period.microseconds();

    /* get sstable cursor */
    auto cursor = reader.getCursor();
    auto body_size = reader.bodySize();
    int row_idx = 0;

    /* status line */
    util::SimpleRateLimitedFn status_line(kMicrosPerSecond, [&] () {
      stx::logInfo(
          "cm.ctrstats",
          "[$1/$2] [$0%] Reading sstable... rows=$3",
          (size_t) ((cursor->position() / (double) body_size) * 100),
          tbl_idx + 1, sstables.size(), row_idx);
    });

    /* read sstable rows */
    for (; cursor->valid(); ++row_idx) {
      status_line.runMaybe();

      auto key = cursor->getKeyString();
      auto val = cursor->getDataBuffer();
      sstable::SSTableColumnReader cols(&schema, val);

      auto num_views = cols.getUInt64Column(schema.columnID("num_views"));
      auto num_clicks = cols.getUInt64Column(schema.columnID("num_clicks"));
      auto num_clicked = cols.getUInt64Column(schema.columnID("num_clicked"));

      auto& counter = (*counters)[key][tbl_window];
      counter.num_views += num_views;
      counter.num_clicks += num_clicks;
      counter.num_clicked += num_clicked;

      if (!cursor->next()) {
        break;
      }
    }

    status_line.runForce();
  }
}

/* write output table */
void writeOutputTable(const String& filename, const Vector<OutputRow>& rows) {
  /* prepare output sstable schema */
  sstable::SSTableColumnSchema sstable_schema;
  sstable_schema.addColumn("time", 1, sstable::SSTableColumnType::UINT64);
  sstable_schema.addColumn("value", 2, sstable::SSTableColumnType::FLOAT);

  /* open output sstable */
  stx::logInfo("cm.ctrstats", "Writing results to: $0", filename);
  auto sstable_writer = sstable::SSTableEditor::create(
      filename,
      sstable::IndexProvider{},
      nullptr,
      0);

  /* write output sstable */
  for (const auto& r : rows) {
    sstable::SSTableColumnWriter cols(&sstable_schema);
    cols.addUInt64Column(1, std::get<1>(r));
    cols.addFloatColumn(2, std::get<2>(r));
    sstable_writer->appendRow(std::get<0>(r), cols);
  }

  sstable_schema.writeIndex(sstable_writer.get());
  sstable_writer->finalize();
}

/* aggregate ctr counters (flat) */
void aggregateCounters(
    CounterMap* counters,
    Duration aggr_period,
    Vector<OutputRow>* rows) {

  for (const auto& row : *counters) {
    const auto& key = row.first;

    for (const auto& t : row.second) {
      rows->emplace_back(
          StringUtil::format("$0~num_views", key),
          t.first * aggr_period.microseconds(),
          t.second.num_views);
    }

    for (const auto& t : row.second) {
      auto num = t.second.num_views;
      auto den = 1;

      for (int i = 0; i < 6; ++i) {
        const auto& v = row.second.find(t.first - i);
        if (v == row.second.end()) continue;
        num += v->second.num_views;
        ++den;
      }

      rows->emplace_back(
          StringUtil::format("$0~num_views_7d", key),
          t.first * aggr_period.microseconds(),
          num / (double) den);
    }

    for (const auto& t : row.second) {
      rows->emplace_back(
          StringUtil::format("$0~num_clicks", key),
          t.first * aggr_period.microseconds(),
          t.second.num_clicks);
    }

    for (const auto& t : row.second) {
      auto num = t.second.num_clicks;
      auto den = 1;

      for (int i = 0; i < 6; ++i) {
        const auto& v = row.second.find(t.first - i);
        if (v == row.second.end()) continue;
        num += v->second.num_clicks;
        ++den;
      }

      rows->emplace_back(
          StringUtil::format("$0~num_clicks_7d", key),
          t.first * aggr_period.microseconds(),
          num / (double) den);
    }

    for (const auto& t : row.second) {
      rows->emplace_back(
          StringUtil::format("$0~ctr", key),
          t.first * aggr_period.microseconds(),
          t.second.num_clicked / (double) t.second.num_views);
    }

    for (const auto& t : row.second) {
      auto num_v = t.second.num_views;
      auto num_c = t.second.num_clicked;
      auto den = 1;

      for (int i = 0; i < 6; ++i) {
        const auto& v = row.second.find(t.first - i);
        if (v == row.second.end()) continue;
        num_v += v->second.num_views;
        num_c += v->second.num_clicked;
        ++den;
      }

      rows->emplace_back(
          StringUtil::format("$0~ctr_7d", key),
          t.first * aggr_period.microseconds(),
          (num_c / (double) den) / (num_v / (double) den));
    }

    for (const auto& t : row.second) {
      rows->emplace_back(
          StringUtil::format("$0~cpq", key),
          t.first * aggr_period.microseconds(),
          t.second.num_clicks / (double) t.second.num_views);
    }

    for (const auto& t : row.second) {
      auto num_v = t.second.num_views;
      auto num_c = t.second.num_clicks;
      auto den = 1;

      for (int i = 0; i < 6; ++i) {
        const auto& v = row.second.find(t.first - i);
        if (v == row.second.end()) continue;
        num_v += v->second.num_views;
        num_c += v->second.num_clicks;
        ++den;
      }

      rows->emplace_back(
          StringUtil::format("$0~cpq_7d", key),
          t.first * aggr_period.microseconds(),
          (num_c / (double) den) / (num_v / (double) den));
    }
  }
}

int main(int argc, const char** argv) {
  stx::Application::init();
  stx::Application::logToStderr();

  stx::cli::FlagParser flags;

  flags.defineFlag(
      "output_file",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "output file path",
      "<path>");

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

  Duration aggr_period(kMicrosPerDay);

  /* read input tables */
  CounterMap counters;
  importInputTables(flags.getArgv(), aggr_period, &counters);

  /* aggregate counters */
  Vector<OutputRow> rows;
  aggregateCounters(&counters, aggr_period, &rows);

  /* write output table */
  writeOutputTable(flags.getString("output_file"), rows);

  return 0;
}

