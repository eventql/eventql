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

using namespace fnord;

int main(int argc, const char** argv) {
  fnord::Application::init();
  fnord::Application::logToStderr();

  fnord::cli::FlagParser flags;

  flags.defineFlag(
      "output_file",
      fnord::cli::FlagParser::T_STRING,
      true,
      "o",
      NULL,
      "output file",
      "<filename>");

  flags.defineFlag(
      "min_observations",
      fnord::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "100",
      "minimum nuber of feature observations",
      "<num>");

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

  auto sstables = flags.getArgv();
  auto tbl_cnt = sstables.size();
  if (tbl_cnt == 0) {
    fnord::logCritical("cm.featuredump", "No input tables, exiting");
    return 1;
  }

  /* read input meta tables and count features */
  HashMap<String, uint64_t> feature_counts;
  for (int tbl_idx = 0; tbl_idx < sstables.size(); ++tbl_idx) {
    String sstable = sstables[tbl_idx];
    StringUtil::replaceAll(&sstable, ".sstable", "_meta.sstable");
    fnord::logInfo("cm.featuredump", "Importing sstable: $0", sstable);

    /* read sstable heade r*/
    sstable::SSTableReader reader(File::openFile(sstable, File::O_READ));
    sstable::SSTableColumnSchema schema;
    schema.loadIndex(&reader);

    if (reader.bodySize() == 0) {
      fnord::logCritical("cm.featuredump", "unfinished sstable: $0", sstable);
      exit(1);
    }

    /* get sstable cursor */
    auto cursor = reader.getCursor();
    auto body_size = reader.bodySize();
    int row_idx = 0;

    /* status line */
    util::SimpleRateLimitedFn status_line(kMicrosPerSecond, [&] () {
      auto p = (tbl_idx / (double) tbl_cnt) +
          ((cursor->position() / (double) body_size)) / (double) tbl_cnt;

      fnord::logInfo(
          "cm.featuredump",
          "[$0%] Reading sstable... rows=$3",
          (size_t) (p * 100), tbl_idx + 1, sstables.size(), row_idx);
    });

    /* read sstable rows */
    for (; cursor->valid(); ++row_idx) {
      status_line.runMaybe();

      auto key = cursor->getKeyString();
      auto val = cursor->getDataBuffer();

      sstable::SSTableColumnReader cols(&schema, val);
      auto cnt = cols.getUInt64Column(schema.columnID("count"));

      feature_counts[key] += cnt;

      if (!cursor->next()) {
        break;
      }
    }

    status_line.runForce();
  }

  /* select features for export */
  Set<String> features;
  auto orig_count = feature_counts.size();
  auto min_count = flags.getInt("min_observations");
  for (auto iter = feature_counts.begin(); iter != feature_counts.end(); ) {
    if (iter->second > min_count) {
      features.emplace(iter->first);
    }

    iter = feature_counts.erase(iter);
  }

  feature_counts.clear();

  fnord::logInfo(
      "cm.featuredump",
      "Exporting $1/$0 features",
      orig_count,
      features.size());

  return 0;
}

