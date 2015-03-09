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

typedef Tuple<
    String,
    String,
    uint64_t,
    uint64_t,
    double,
    double,
    double,
    double> OutputRow;

typedef HashMap<String, cm::CTRCounter> CounterMap;

/* read all input sstables */
void importInputTables(const Vector<String> sstables, CounterMap* counters) {
  for (int tbl_idx = 0; tbl_idx < sstables.size(); ++tbl_idx) {
    const auto& sstable = sstables[tbl_idx];
    fnord::logInfo("cm.ctrstats", "Importing sstable: $0", sstable);

    /* read sstable heade r*/
    sstable::SSTableReader reader(File::openFile(sstable, File::O_READ));
    sstable::SSTableColumnSchema schema;
    schema.loadIndex(&reader);

    if (reader.bodySize() == 0) {
      fnord::logCritical("cm.ctrstats", "unfinished sstable: $0", sstable);
      exit(1);
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

      auto key = cursor->getKeyString();
      auto val = cursor->getDataBuffer();
      sstable::SSTableColumnReader cols(&schema, val);

      auto& counter = (*counters)[key];
      auto num_views = cols.getUInt64Column(schema.columnID("num_views"));
      auto num_clicks= cols.getUInt64Column(schema.columnID("num_clicks"));
      counter.num_views += num_views;
      counter.num_clicks += num_clicks;

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
  sstable_schema.addColumn("num_views", 1, sstable::SSTableColumnType::UINT64);
  sstable_schema.addColumn("num_clicks", 2, sstable::SSTableColumnType::UINT64);
  sstable_schema.addColumn("p_view_base", 3, sstable::SSTableColumnType::FLOAT);
  sstable_schema.addColumn("p_view_dim1", 4, sstable::SSTableColumnType::FLOAT);
  sstable_schema.addColumn("p_click", 5, sstable::SSTableColumnType::FLOAT);
  sstable_schema.addColumn(
      "p_click_normalized",
      6,
      sstable::SSTableColumnType::FLOAT);

  /* open output sstable */
  fnord::logInfo("cm.ctrstats", "Writing results to: $0", filename);
  auto sstable_writer = sstable::SSTableWriter::create(
      filename,
      sstable::IndexProvider{},
      nullptr,
      0);

  /* write output sstable */
  for (const auto& r : rows) {
    sstable::SSTableColumnWriter cols(&sstable_schema);

    cols.addStringColumn(1, std::get<1>(r));
    cols.addUInt64Column(2, std::get<2>(r));
    cols.addUInt64Column(3, std::get<3>(r));
    cols.addFloatColumn(4, std::get<4>(r));
    cols.addFloatColumn(5, std::get<5>(r));
    cols.addFloatColumn(6, std::get<6>(r));
    cols.addFloatColumn(7, std::get<7>(r));

    sstable_writer->appendRow(std::get<0>(r), cols);
  }

  sstable_schema.writeIndex(sstable_writer.get());
  sstable_writer->finalize();
}

/* aggregate ctr counters (flat) */
void aggregateCounters(CounterMap* counters, Vector<OutputRow>* rows) {
  const auto& global_counter_iter = counters->find("__GLOBAL");
  if (global_counter_iter == counters->end()) {
    fnord::logCritical("cm.ctrstatsexport", "missing global counter");
    exit(1);
  }
  const auto& global_counter = global_counter_iter->second;

  for (const auto& row : *counters) {
    const auto& ctr = row.second;

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
      fnord::logWarning("cm.ctrstatsexport", "missing row: $0", dim1);
      continue;
    }
    const auto& group_counter = group_counter_iter->second;

    double p_base = ctr.num_views / (double) global_counter.num_views;
    double p_base_dim2 = ctr.num_views / (double) group_counter.num_views;
    double p_click = ctr.num_clicks / (double) ctr.num_views;
    double p_click_n = ctr.num_clicks / (double) group_counter.num_clicks;

    rows->emplace_back(
        dim1,
        dim2,
        row.second.num_views,
        row.second.num_clicks,
        p_base,
        p_base_dim2,
        p_click,
        p_click_n);
  }
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

  /* read input tables */
  HashMap<String, cm::CTRCounter> counters;
  importInputTables(flags.getArgv(), &counters);

  /* aggregate counters */
  Vector<OutputRow> rows;
  aggregateCounters(&counters, &rows);

  /* write output table */
  writeOutputTable(flags.getString("output_file"), rows);

  return 0;
}

