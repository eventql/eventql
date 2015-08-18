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
#include "stx/cli/flagparser.h"
#include "stx/util/SimpleRateLimit.h"
#include "stx/mdb/MDB.h"
#include "stx/mdb/MDBUtil.h"
#include "sstable/sstablereader.h"
#include "sstable/SSTableEditor.h"
#include "sstable/SSTableColumnSchema.h"
#include "sstable/SSTableColumnReader.h"
#include "sstable/SSTableColumnWriter.h"
#include "common.h"
#include "CustomerNamespace.h"

#
#include "zbase/CTRCounter.h"

using namespace stx;

typedef Tuple<
    String,
    uint64_t,
    uint64_t,
    uint64_t,
    double,
    double,
    double,
    double,
    double,
    double> OutputRow;
typedef HashMap<String, cm::CTRCounterData> CounterMap;

/* read all input sstables */
void importInputTables(const Vector<String> sstables, CounterMap* counters) {
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

      auto& counter = (*counters)[key];
      auto num_views = cols.getUInt64Column(schema.columnID("num_views"));
      auto num_clicks = cols.getUInt64Column(schema.columnID("num_clicks"));
      auto num_clicked = cols.getUInt64Column(schema.columnID("num_clicked"));
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
  sstable::SSTableColumnSchema schema;
  schema.addColumn("num_views", 1, sstable::SSTableColumnType::UINT64);
  schema.addColumn("num_clicks", 2, sstable::SSTableColumnType::UINT64);
  schema.addColumn("num_clicked", 3, sstable::SSTableColumnType::UINT64);
  schema.addColumn("ctr", 4, sstable::SSTableColumnType::FLOAT);
  schema.addColumn("cpq", 5, sstable::SSTableColumnType::FLOAT);
  schema.addColumn("p_view_base", 6, sstable::SSTableColumnType::FLOAT);
  schema.addColumn("p_click_base", 7, sstable::SSTableColumnType::FLOAT);
  schema.addColumn("p_clicked_base", 8, sstable::SSTableColumnType::FLOAT);
  schema.addColumn("perf_base", 9, sstable::SSTableColumnType::FLOAT);

  /* open output sstable */
  stx::logInfo("cm.ctrstats", "Writing results to: $0", filename);
  auto sstable_writer = sstable::SSTableEditor::create(
      filename,
      sstable::IndexProvider{},
      nullptr,
      0);

  /* write output sstable */
  for (const auto& r : rows) {
    sstable::SSTableColumnWriter cols(&schema);

    cols.addUInt64Column(1, std::get<1>(r));
    cols.addUInt64Column(2, std::get<2>(r));
    cols.addUInt64Column(3, std::get<3>(r));
    cols.addFloatColumn(4, std::get<4>(r));
    cols.addFloatColumn(5, std::get<5>(r));
    cols.addFloatColumn(6, std::get<6>(r));
    cols.addFloatColumn(7, std::get<7>(r));
    cols.addFloatColumn(8, std::get<8>(r));
    cols.addFloatColumn(9, std::get<9>(r));

    sstable_writer->appendRow(std::get<0>(r), cols);
  }

  schema.writeIndex(sstable_writer.get());
  sstable_writer->finalize();
}

/* aggregate ctr counters (flat) */
void aggregateCounters(CounterMap* counters, Vector<OutputRow>* rows) {
  const auto& global_counter_iter = counters->find("__GLOBAL");
  if (global_counter_iter == counters->end()) {
    stx::logCritical("cm.ctrstats", "missing global counter");
    exit(1);
  }
  const auto& global_counter = global_counter_iter->second;

  for (const auto& row : *counters) {
    const auto& c = row.second;
    if (row.first.length() == 0 || row.first == "__GLOBAL") {
      continue;
    }

    double ctr = c.num_clicked / (double) c.num_views;
    double cpq = c.num_clicks / (double) c.num_views;
    double p_view_base = c.num_views / (double) global_counter.num_views;
    double p_click_base = c.num_clicks / (double) global_counter.num_clicks;
    double p_clicked_base = c.num_clicked / (double) global_counter.num_clicked;
    double perf_base = p_click_base / p_view_base;

    rows->emplace_back(
        row.first,
        row.second.num_views,
        row.second.num_clicks,
        row.second.num_clicked,
        ctr,
        cpq,
        p_view_base,
        p_click_base,
        p_clicked_base,
        perf_base);
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

  /* read input tables */
  HashMap<String, cm::CTRCounterData> counters;
  importInputTables(flags.getArgv(), &counters);

  /* aggregate counters */
  Vector<OutputRow> rows;
  aggregateCounters(&counters, &rows);

  /* write output table */
  writeOutputTable(flags.getString("output_file"), rows);

  return 0;
}

