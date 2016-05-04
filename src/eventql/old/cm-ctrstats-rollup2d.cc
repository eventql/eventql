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
#include "eventql/util/mdb/MDB.h"
#include "eventql/util/mdb/MDBUtil.h"
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

typedef Tuple<
    String,
    String,
    uint64_t,
    uint64_t,
    uint64_t,
    double,
    double,
    double,
    double,
    double,
    double,
    double,
    double,
    double,
    double> OutputRow;

typedef HashMap<String, zbase::CTRCounterData> CounterMap;

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
  schema.addColumn("dim1", 1, sstable::SSTableColumnType::STRING);
  schema.addColumn("num_views", 2, sstable::SSTableColumnType::UINT64);
  schema.addColumn("num_clicks", 3, sstable::SSTableColumnType::UINT64);
  schema.addColumn("num_clicked", 4, sstable::SSTableColumnType::UINT64);
  schema.addColumn("ctr", 5, sstable::SSTableColumnType::FLOAT);
  schema.addColumn("cpq", 6, sstable::SSTableColumnType::FLOAT);
  schema.addColumn("p_view_base", 7, sstable::SSTableColumnType::FLOAT);
  schema.addColumn("p_click_base", 8, sstable::SSTableColumnType::FLOAT);
  schema.addColumn("p_clicked_base", 9, sstable::SSTableColumnType::FLOAT);
  schema.addColumn("p_view_dim1", 10, sstable::SSTableColumnType::FLOAT);
  schema.addColumn("p_click_dim1", 11, sstable::SSTableColumnType::FLOAT);
  schema.addColumn("p_clicked_dim1", 12, sstable::SSTableColumnType::FLOAT);
  schema.addColumn("perf_base", 13, sstable::SSTableColumnType::FLOAT);
  schema.addColumn("perf_dim1", 14, sstable::SSTableColumnType::FLOAT);

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

    cols.addStringColumn(1, std::get<1>(r));
    cols.addUInt64Column(2, std::get<2>(r));
    cols.addUInt64Column(3, std::get<3>(r));
    cols.addUInt64Column(4, std::get<4>(r));
    cols.addFloatColumn(5, std::get<5>(r));
    cols.addFloatColumn(6, std::get<6>(r));
    cols.addFloatColumn(7, std::get<7>(r));
    cols.addFloatColumn(8, std::get<8>(r));
    cols.addFloatColumn(9, std::get<9>(r));
    cols.addFloatColumn(10, std::get<10>(r));
    cols.addFloatColumn(11, std::get<11>(r));
    cols.addFloatColumn(12, std::get<12>(r));
    cols.addFloatColumn(13, std::get<13>(r));
    cols.addFloatColumn(14, std::get<14>(r));

    sstable_writer->appendRow(std::get<0>(r), cols);
  }

  schema.writeIndex(sstable_writer.get());
  sstable_writer->finalize();
}

/* aggregate ctr counters (flat) */
void aggregateCounters(
    const cli::FlagParser& flags,
    CounterMap* counters,
    Vector<OutputRow>* rows) {
  long min_views = flags.isSet("min_views") ? flags.getInt("min_views") : -1;
  long min_clicks = flags.isSet("min_clicks") ? flags.getInt("min_clicks") : -1;

  const auto& global_counter_iter = counters->find("__GLOBAL");
  if (global_counter_iter == counters->end()) {
    stx::logCritical("cm.ctrstatsexport", "missing global counter");
    exit(1);
  }
  const auto& global_counter = global_counter_iter->second;

  for (const auto& row : *counters) {
    const auto& c = row.second;

    if (min_views > 0 && c.num_views < min_views) {
      continue;
    }

    if (min_clicks > 0 && c.num_clicks < min_clicks) {
      continue;
    }

    auto sep = row.first.find("~");
    if (sep == std::string::npos) {
      continue;
    }
    auto dim1 = row.first.substr(0, sep);
    auto dim2 = row.first.substr(sep + 1);

    if (dim1.length() == 0 || dim2.length() == 0) {
      continue;
    }

    const auto& group_counter_iter = counters->find(dim1);
    if (group_counter_iter == counters->end()) {
      stx::logWarning("cm.ctrstatsexport", "missing row: $0", dim1);
      continue;
    }
    const auto& group_counter = group_counter_iter->second;

    double ctr = c.num_clicked / (double) c.num_views;
    double cpq = c.num_clicks / (double) c.num_views;
    double p_view_base = c.num_views / (double) global_counter.num_views;
    double p_click_base = c.num_clicks / (double) global_counter.num_clicks;
    double p_clicked_base = c.num_clicked / (double) global_counter.num_clicked;
    double perf_base = p_click_base / p_view_base;
    double p_view_dim1 = c.num_views / (double) group_counter.num_views;
    double p_click_dim1 = c.num_clicks / (double) group_counter.num_clicks;
    double p_clicked_dim1 = c.num_clicked / (double) group_counter.num_clicked;
    double perf_dim1 = p_click_dim1 / p_view_dim1;

    rows->emplace_back(
        dim1,
        dim2,
        row.second.num_views,
        row.second.num_clicks,
        row.second.num_clicked,
        ctr,
        cpq,
        p_view_base,
        p_click_base,
        p_clicked_base,
        p_view_dim1,
        p_click_dim1,
        p_clicked_dim1,
        perf_base,
        perf_dim1);
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
      "min_views",
      cli::FlagParser::T_INTEGER,
      false,
      NULL,
      NULL,
      "min views",
      "<num>");

  flags.defineFlag(
      "min_clicks",
      cli::FlagParser::T_INTEGER,
      false,
      NULL,
      NULL,
      "min clicks",
      "<num>");

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
  HashMap<String, zbase::CTRCounterData> counters;
  importInputTables(flags.getArgv(), &counters);

  /* aggregate counters */
  Vector<OutputRow> rows;
  aggregateCounters(flags, &counters, &rows);

  /* write output table */
  writeOutputTable(flags.getString("output_file"), rows);

  return 0;
}

