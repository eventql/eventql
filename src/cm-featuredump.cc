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
#include "PackedExample.h"

using namespace fnord;

int main(int argc, const char** argv) {
  fnord::Application::init();
  fnord::Application::logToStderr();

  fnord::cli::FlagParser flags;

  flags.defineFlag(
      "output_lightsvm",
      fnord::cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "output lightsvm data",
      "<filename>");

  flags.defineFlag(
      "output_meta",
      fnord::cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "output feature metadata",
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

  /* open output: meta file */
  Option<File> output_meta_file;
  String output_meta_file_path;

  if (flags.isSet("output_meta")) {
    output_meta_file_path = flags.getString("output_meta");

    fnord::logInfo(
        "cm.featureprep",
        "Writing feature metadata to: $0",
        output_meta_file_path);

    if (FileUtil::exists(output_meta_file_path + "~")) {
      fnord::logInfo(
          "cm.featuredump",
          "Deleting orphaned tmp file: $0",
          output_meta_file_path + "~");

      FileUtil::rm(output_meta_file_path + "~");
    }

    output_meta_file = std::move(Option<File>(std::move(
        File::openFile(
            output_meta_file_path + "~",
            File::O_READ | File::O_WRITE | File::O_CREATE))));
  }

  /* open output: lightsvm file */
  Option<File> output_lightsvm_file;
  String output_lightsvm_file_path;

  if (flags.isSet("output_lightsvm")) {
    output_lightsvm_file_path = flags.getString("output_lightsvm");

    fnord::logInfo(
        "cm.featureprep",
        "Writing lightSVM features to: $0",
        output_lightsvm_file_path);

    if (FileUtil::exists(output_lightsvm_file_path + "~")) {
      fnord::logInfo(
          "cm.featuredump",
          "Deleting orphaned tmp file: $0",
          output_lightsvm_file_path + "~");

      FileUtil::rm(output_lightsvm_file_path + "~");
    }

    output_lightsvm_file = std::move(Option<File>(std::move(
        File::openFile(
            output_lightsvm_file_path + "~",
            File::O_READ | File::O_WRITE | File::O_CREATE))));
  }

  /* read input meta tables and count features */
  HashMap<String, uint64_t> feature_counts;
  int row_idx = 0;
  for (int tbl_idx = 0; tbl_idx < sstables.size(); ++tbl_idx) {
    String sstable = sstables[tbl_idx];
    StringUtil::replaceAll(&sstable, ".sstable", "_meta.sstable");
    fnord::logInfo("cm.featuredump", "Importing meta sstable: $0", sstable);

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

    /* status line */
    util::SimpleRateLimitedFn status_line(kMicrosPerSecond, [&] () {
      auto p = (tbl_idx / (double) tbl_cnt) +
          ((cursor->position() / (double) body_size)) / (double) tbl_cnt;

      fnord::logInfo(
          "cm.featuredump",
          "[$0%] Reading meta sstables... rows=$1 features=$2",
          (size_t) (p * 100),
          row_idx,
          feature_counts.size());
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

  /* select features for export and assign indexes*/
  HashMap<String, uint64_t> feature_idx;
  uint64_t cur_idx = 0;
  auto orig_count = feature_counts.size();
  auto min_count = flags.getInt("min_observations");
  for (auto iter = feature_counts.begin(); iter != feature_counts.end(); ) {
    if (iter->second > min_count) {
      feature_idx.emplace(iter->first, ++cur_idx);
    }

    iter = feature_counts.erase(iter);
  }

  feature_counts.clear();

  fnord::logInfo(
      "cm.featuredump",
      "Exporting $1/$0 features",
      orig_count,
      feature_idx.size());

  /* write feature idx meta file */
  if (!output_meta_file.isEmpty()) {
    for (const auto& f : feature_idx) {
      auto line = StringUtil::format("$1 $0\n", f.first, f.second);
      output_meta_file.get().write(line);
    }
  }

  /* read input data tables */
  int rows_read = 0;
  int rows_written = 0;
  for (int tbl_idx = 0; tbl_idx < sstables.size(); ++tbl_idx) {
    const auto& sstable = sstables[tbl_idx];
    fnord::logInfo("cm.featuredump", "Importing data sstable: $0", sstable);

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

    /* status line */
    util::SimpleRateLimitedFn status_line(kMicrosPerSecond, [&] () {
      auto p = (tbl_idx / (double) tbl_cnt) +
          ((cursor->position() / (double) body_size)) / (double) tbl_cnt;

      fnord::logInfo(
          "cm.featuredump",
          "[$0%] Reading data sstables... rows_read=$1 rows_written=$2",
          (size_t) (p * 100),
          rows_read,
          rows_written);
    });

    /* read data sstables and copy features */
    for (; cursor->valid(); ++rows_read) {
      status_line.runMaybe();

      auto val = cursor->getDataBuffer();
      sstable::SSTableColumnReader cols(&schema, val);

      cm::Example ex;
      ex.label = cols.getFloatColumn(schema.columnID("label"));
      auto features = cols.getStringColumns(schema.columnID("feature"));
      for (const auto& f : features) {
        auto idx = feature_idx.find(f);
        if (idx == feature_idx.end()) {
          continue;
        }

        ex.features.emplace_back(idx->second, 1.0);
      }

      if (!output_lightsvm_file.isEmpty() && ex.features.size() > 0) {
        auto ex_str = exampleToSVMLight(ex);
        output_lightsvm_file.get().write(ex_str + "\n");
        ++rows_written;
      }

      if (!cursor->next()) {
        break;
      }
    }

    status_line.runForce();
  }

  if (!output_meta_file.isEmpty()) {
    FileUtil::mv(output_meta_file_path + "~", output_meta_file_path);
  }

  if (!output_lightsvm_file.isEmpty()) {
    FileUtil::mv(output_lightsvm_file_path + "~", output_lightsvm_file_path);
  }

  return 0;
}

