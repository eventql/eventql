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
#include "stx/InternMap.h"
#include "stx/json/json.h"
#include "eventql/util/mdb/MDB.h"
#include "eventql/util/mdb/MDBUtil.h"
#include "eventql/infra/sstable/sstablereader.h"
#include "eventql/infra/sstable/SSTableEditor.h"
#include "eventql/infra/sstable/SSTableColumnSchema.h"
#include "eventql/infra/sstable/SSTableColumnReader.h"
#include "eventql/infra/sstable/SSTableColumnWriter.h"
#include "common.h"
#include "PackedExample.h"

using namespace stx;

int main(int argc, const char** argv) {
  stx::Application::init();
  stx::Application::logToStderr();

  stx::cli::FlagParser flags;

  flags.defineFlag(
      "input_meta",
      stx::cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "read selected features from file",
      "<filename>");

  flags.defineFlag(
      "output_lightsvm",
      stx::cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "output lightsvm data",
      "<filename>");

  flags.defineFlag(
      "output_mm_labels",
      stx::cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "output matrix market labels",
      "<filename>");

  flags.defineFlag(
      "output_mm_features",
      stx::cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "output matrix market features",
      "<filename>");

  flags.defineFlag(
      "output_meta",
      stx::cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "output feature metadata",
      "<filename>");

  flags.defineFlag(
      "min_observations",
      stx::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "100",
      "minimum nuber of feature observations",
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

  auto sstables = flags.getArgv();
  auto tbl_cnt = sstables.size();
  if (tbl_cnt == 0) {
    stx::logCritical("cm.featuredump", "No input tables, exiting");
    return 1;
  }

  /* open output: meta file */
  Option<File> output_meta_file;
  String output_meta_file_path;

  if (flags.isSet("output_meta")) {
    output_meta_file_path = flags.getString("output_meta");

    stx::logInfo(
        "cm.featureprep",
        "Writing feature metadata to: $0",
        output_meta_file_path);

    output_meta_file = std::move(Option<File>(std::move(
        File::openFile(
            output_meta_file_path + "~",
            File::O_WRITE | File::O_CREATEOROPEN | File::O_TRUNCATE))));
  }

  /* open output: lightsvm file */
  Option<File> output_lightsvm_file;
  String output_lightsvm_file_path;

  if (flags.isSet("output_lightsvm")) {
    output_lightsvm_file_path = flags.getString("output_lightsvm");

    stx::logInfo(
        "cm.featureprep",
        "Writing lightSVM features to: $0",
        output_lightsvm_file_path);

    output_lightsvm_file = std::move(Option<File>(std::move(
        File::openFile(
            output_lightsvm_file_path + "~",
            File::O_WRITE | File::O_CREATEOROPEN | File::O_TRUNCATE))));
  }

  /* open output: matrix market labels file */
  Option<File> output_mm_labels_file;
  String output_mm_labels_file_path;

  if (flags.isSet("output_mm_labels")) {
    output_mm_labels_file_path = flags.getString("output_mm_labels");

    stx::logInfo(
        "cm.featureprep",
        "Writing matrix market labels to $0",
        output_mm_labels_file_path);

    output_mm_labels_file = std::move(Option<File>(std::move(
        File::openFile(
            output_mm_labels_file_path + "~",
            File::O_WRITE | File::O_CREATEOROPEN | File::O_TRUNCATE))));
  }

  /* open output: matrix market features file */
  Option<File> output_mm_features_file;
  String output_mm_features_file_path;

  if (flags.isSet("output_mm_features")) {
    output_mm_features_file_path = flags.getString("output_mm_features");

    stx::logInfo(
        "cm.featureprep",
        "Writing matrix market features to $0",
        output_mm_features_file_path);

    output_mm_features_file = std::move(Option<File>(std::move(
        File::openFile(
            output_mm_features_file_path + "~",
            File::O_WRITE | File::O_CREATEOROPEN | File::O_TRUNCATE))));
  }


  /* select features / build or load feature idx */
  HashMap<String, uint64_t> feature_idx;
  size_t orig_count = 0;
  if (flags.isSet("input_meta")) {
    // FIXPAUL slow slow slow
    auto buf = FileUtil::read(flags.getString("input_meta"));
    auto lines = StringUtil::split(buf.toString(), "\n");

    for (const auto& line : lines) {
      if (line.length() < 3) {
        continue;
      }

      auto parts = StringUtil::split(line, " ");

      if (parts.size() != 2) {
        RAISEF(kRuntimeError, "invalid feature meta file line: $0", line);
      }

      feature_idx[parts[1]] = std::stoul(parts[0]);
    }

    orig_count = feature_idx.size();
  } else {
    /* read input meta tables and count features */
    HashMap<String, uint64_t> feature_counts;
    int row_idx = 0;
    for (int tbl_idx = 0; tbl_idx < sstables.size(); ++tbl_idx) {
      String sstable = sstables[tbl_idx];
      StringUtil::replaceAll(&sstable, ".sstable", "_meta.sstable");
      stx::logInfo("cm.featuredump", "Importing meta sstable: $0", sstable);

      /* read sstable heade r*/
      sstable::SSTableReader reader(File::openFile(sstable, File::O_READ));
      sstable::SSTableColumnSchema schema;
      schema.loadIndex(&reader);

      if (reader.bodySize() == 0) {
        stx::logCritical("cm.featuredump", "unfinished sstable: $0", sstable);
        exit(1);
      }

      /* get sstable cursor */
      auto cursor = reader.getCursor();
      auto body_size = reader.bodySize();

      /* status line */
      util::SimpleRateLimitedFn status_line(kMicrosPerSecond, [&] () {
        auto p = (tbl_idx / (double) tbl_cnt) +
            ((cursor->position() / (double) body_size)) / (double) tbl_cnt;

        stx::logInfo(
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
        auto cnt = cols.getUInt64Column(schema.columnID("views"));

        feature_counts[key] += cnt;

        if (!cursor->next()) {
          break;
        }
      }

      status_line.runForce();
    }

    /* select features for export and assign indexes*/
    uint64_t cur_idx = 0;
    orig_count = feature_counts.size();
    auto min_count = flags.getInt("min_observations");
    for (auto iter = feature_counts.begin(); iter != feature_counts.end(); ) {
      if (iter->second > min_count) {
        feature_idx.emplace(iter->first, ++cur_idx);
      }

      iter = feature_counts.erase(iter);
    }
  }

  stx::logInfo(
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
  int features_written = 0;
  for (int tbl_idx = 0; tbl_idx < sstables.size(); ++tbl_idx) {
    const auto& sstable = sstables[tbl_idx];
    stx::logInfo("cm.featuredump", "Importing data sstable: $0", sstable);

    /* read sstable heade r*/
    sstable::SSTableReader reader(File::openFile(sstable, File::O_READ));
    sstable::SSTableColumnSchema schema;
    schema.loadIndex(&reader);

    if (reader.bodySize() == 0) {
      stx::logCritical("cm.featuredump", "unfinished sstable: $0", sstable);
      exit(1);
    }

    /* get sstable cursor */
    auto cursor = reader.getCursor();
    auto body_size = reader.bodySize();

    /* status line */
    util::SimpleRateLimitedFn status_line(kMicrosPerSecond, [&] () {
      auto p = (tbl_idx / (double) tbl_cnt) +
          ((cursor->position() / (double) body_size)) / (double) tbl_cnt;

      stx::logInfo(
          "cm.featuredump",
          "[$0%] Dumping features... rows_read=$1 rows_written=$2",
          (size_t) (p * 100),
          rows_read,
          rows_written);
    });

    /* read data sstables and copy features */
    for (; cursor->valid(); ++rows_read) {
      status_line.runMaybe();

      auto val = cursor->getDataBuffer();
      sstable::SSTableColumnReader cols(&schema, val);

      zbase::Example ex;
#ifndef FNORD_NODEBUG
      Set<String> dbg_features;
#endif
      ex.label = cols.getFloatColumn(schema.columnID("label"));
      auto features = cols.getStringColumns(schema.columnID("feature"));
      for (const auto& f : features) {
        auto idx = feature_idx.find(f);
        if (idx == feature_idx.end()) {
          continue;
        }

#ifndef FNORD_NODEBUG
        dbg_features.emplace(f);
#endif

        ex.features.emplace_back(idx->second, 1.0);
      }

      if (ex.features.size() > 0) {
        ++rows_written;
        features_written += ex.features.size();

#ifndef FNORD_NOTRACE
        stx::logTrace(
            "cm.featuredump",
            "Dumping example: label=$0 features=$1",
            ex.label,
            inspect(dbg_features));
#endif

        if (!output_lightsvm_file.isEmpty()) {
          auto ex_str = exampleToSVMLight(ex);
          output_lightsvm_file.get().write(ex_str + "\n");
        }

        if (!output_mm_labels_file.isEmpty()) {
          auto line = StringUtil::format("$0\n", ex.label);
          output_mm_labels_file.get().write(line);
        }

        if (!output_mm_features_file.isEmpty()) {
          String lines;
          ex.sortFeatures();
          for (const auto& f : ex.features) {
            lines += StringUtil::format(
                "$0 $1 $2\n",
                rows_written,
                f.first,
                f.second);
          }
          output_mm_features_file.get().write(lines);
        }
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

  if (!output_mm_labels_file.isEmpty()) {
    auto tmpfile = File::openFile(
        output_mm_labels_file_path + "~~",
        File::O_READ | File::O_WRITE | File::O_CREATEOROPEN | File::O_TRUNCATE);

    tmpfile.write(StringUtil::format(
        "%%MatrixMarket matrix array real general\n$0 1\n",
        rows_written));

    FileUtil::cat(
        output_mm_labels_file_path + "~",
        output_mm_labels_file_path + "~~");

    FileUtil::mv(output_mm_labels_file_path + "~~", output_mm_labels_file_path);
    FileUtil::rm(output_mm_labels_file_path + "~");
  }

  if (!output_mm_features_file.isEmpty()) {
    auto tmpfile = File::openFile(
        output_mm_features_file_path + "~~",
        File::O_READ | File::O_WRITE | File::O_CREATEOROPEN | File::O_TRUNCATE);

    tmpfile.write(StringUtil::format(
        "%%MatrixMarket matrix coordinate real general\n$0 $1 $2\n",
        rows_written,
        feature_idx.size(),
        features_written));

    FileUtil::cat(
        output_mm_features_file_path + "~",
        output_mm_features_file_path + "~~");

    FileUtil::mv(output_mm_features_file_path + "~~", output_mm_features_file_path);
    FileUtil::rm(output_mm_features_file_path + "~");
  }

  return 0;
}

