/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include "fnord-base/io/filerepository.h"
#include "fnord-base/io/fileutil.h"
#include "fnord-base/io/mmappedfile.h"
#include "fnord-base/application.h"
#include "fnord-base/logging.h"
#include "fnord-base/random.h"
#include "fnord-base/thread/eventloop.h"
#include "fnord-base/thread/threadpool.h"
#include "fnord-base/wallclock.h"
#include "fnord-base/util/SimpleRateLimit.h"
#include "fnord-rpc/ServerGroup.h"
#include "fnord-rpc/RPC.h"
#include "fnord-rpc/RPCClient.h"
#include "fnord-base/cli/flagparser.h"
#include "fnord-json/json.h"
#include "fnord-json/jsonrpc.h"
#include "fnord-http/httprouter.h"
#include "fnord-http/httpserver.h"
#include "fnord-sstable/sstablereader.h"
#include "fnord-sstable/sstablewriter.h"
#include "fnord-sstable/SSTableColumnSchema.h"
#include "fnord-sstable/SSTableColumnReader.h"
#include "fnord-sstable/SSTableColumnWriter.h"
#include "fnord-feeds/FeedService.h"
#include "fnord-feeds/RemoteFeedFactory.h"
#include "fnord-feeds/RemoteFeedReader.h"
#include "fnord-base/stats/statsdagent.h"
#include "fnord-base/RadixTree.h"
#include "fnord-mdb/MDB.h"
#include "CustomerNamespace.h"
#include "FeatureSchema.h"
#include "FeaturePack.h"
#include "FeatureIndex.h"
#include "FeatureSelector.h"
#include "JoinedQuery.h"

using namespace fnord;

int main(int argc, const char** argv) {
  fnord::Application::init();
  fnord::Application::logToStderr();

  fnord::cli::FlagParser flags;

  flags.defineFlag(
      "input_file",
      fnord::cli::FlagParser::T_STRING,
      true,
      "i",
      NULL,
      "input file",
      "<filename>");

  flags.defineFlag(
      "output_file",
      fnord::cli::FlagParser::T_STRING,
      true,
      "o",
      NULL,
      "output file",
      "<filename>");

  flags.defineFlag(
      "featuredb_path",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "feature db path",
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

  /* set up feature schema */
  cm::FeatureSchema feature_schema;
  feature_schema.registerFeature("shop_id", 1, 1);
  feature_schema.registerFeature("category1", 2, 1);
  feature_schema.registerFeature("category2", 3, 1);
  feature_schema.registerFeature("category3", 4, 1);

  /* open featuredb */
  auto input_file = flags.getString("input_file");
  auto featuredb_path = flags.getString("featuredb_path");
  auto featuredb = mdb::MDB::open(featuredb_path, true);
  cm::FeatureIndex feature_index(featuredb, &feature_schema);

  /* feature selector */
  cm::FeatureSelector feature_select(&feature_index);
  HashMap<String, uint64_t> feature_counts;

  /* read input table */
  fnord::logInfo("cm.featureselect", "Importing sstable: $0", input_file);
  sstable::SSTableReader reader(File::openFile(input_file, File::O_READ));

  if (reader.bodySize() == 0) {
    fnord::logCritical("cm.featureselect", "unfinished table: $0", input_file);
    return 1;
  }

  /* get sstable cursor */
  auto cursor = reader.getCursor();
  auto body_size = reader.bodySize();
  int row_idx = 0;

  /* status line */
  util::SimpleRateLimitedFn status_line(kMicrosPerSecond, [&] () {
    fnord::logInfo(
        "cm.featureselect",
        "[$0%] Reading sstable... rows=$1",
        (size_t) ((cursor->position() / (double) body_size) * 100),
        row_idx);
  });

  /* read sstable rows */
  for (; cursor->valid(); ++row_idx) {
    status_line.runMaybe();

    auto val = cursor->getDataBuffer();

    try {
      auto q = json::fromJSON<cm::JoinedQuery>(val);

      for (const auto& item : q.items) {
        Set<String> features;
        feature_select.featuresFor(q, item, &features);
        for (const auto& f : features) {
          ++feature_counts[f];
        }
      }
    } catch (const Exception& e) {
      fnord::logWarning(
          "cm.featureselect",
          e,
          "error while indexing query: $0",
          val.toString());
    }

    if (!cursor->next()) {
      break;
    }
  }

  status_line.runForce();

  /* write output sstable */
  sstable::SSTableColumnSchema sstable_schema;
  sstable_schema.addColumn("count", 1, sstable::SSTableColumnType::UINT64);

  auto output_file = flags.getString("output_file");
  fnord::logInfo("cm.featureselect", "Writing results to: $0", output_file);
  auto sstable_writer = sstable::SSTableWriter::create(
      output_file,
      sstable::IndexProvider{},
      NULL,
      0);

  for (const auto& f : feature_counts) {
    sstable::SSTableColumnWriter cols(&sstable_schema);
    cols.addUInt64Column(1, f.second);
    sstable_writer->appendRow(f.first, cols);
  }

  sstable_schema.writeIndex(sstable_writer.get());
  sstable_writer->finalize();

  return 0;
}

