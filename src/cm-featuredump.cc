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
#include "fnord-rpc/ServerGroup.h"
#include "fnord-rpc/RPC.h"
#include "fnord-rpc/RPCClient.h"
#include "fnord-base/cli/flagparser.h"
#include "fnord-json/json.h"
#include "fnord-json/jsonrpc.h"
#include "fnord-http/httprouter.h"
#include "fnord-http/httpserver.h"
#include "fnord-feeds/FeedService.h"
#include "fnord-feeds/RemoteFeedFactory.h"
#include "fnord-feeds/RemoteFeedReader.h"
#include "fnord-base/stats/statsdagent.h"
#include "fnord-base/RadixTree.h"
#include "fnord-mdb/MDB.h"
#include "CustomerNamespace.h"
#include "FeatureSchema.h"
#include "FeaturePack.h"
#include "PackedExample.h"
#include "FeatureIndex.h"
#include "JoinedQuery.h"

using namespace fnord;

int main(int argc, const char** argv) {
  fnord::Application::init();
  fnord::Application::logToStderr();

  fnord::cli::FlagParser flags;

  flags.defineFlag(
      "cmdata",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "clickmatcher app data dir",
      "<path>");

  flags.defineFlag(
      "cmcustomer",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "clickmatcher customer",
      "<key>");

  flags.defineFlag(
      "features",
      fnord::cli::FlagParser::T_STRING,
      true,
      "f",
      NULL,
      "features file",
      "<filename>");

  flags.defineFlag(
      "input",
      fnord::cli::FlagParser::T_STRING,
      true,
      "i",
      NULL,
      "input file",
      "<filename>");

  flags.defineFlag(
      "output",
      fnord::cli::FlagParser::T_STRING,
      true,
      "o",
      NULL,
      "output file",
      "<filename>");

  flags.defineFlag(
      "separator",
      fnord::cli::FlagParser::T_STRING,
      false,
      "s",
      "\n",
      "separator",
      "<char>");

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

  /* args */
  auto cmcustomer = flags.getString("cmcustomer");
  auto featurefile_path = flags.getString("features");
  auto inputfile_path = flags.getString("input");
  auto outputfile_path = flags.getString("output");

  auto separator_str = flags.getString("separator");
  if (separator_str.length() != 1) {
    // print usage
    return 1;
  }

  char separator = separator_str[0];

  /* set up and read output feature schema */
  cm::FeatureSchema output_feature_schema;
  output_feature_schema.load(FileUtil::read(featurefile_path));

  /* set up featuredb schema */
  cm::FeatureSchema featuredb_schema;
  featuredb_schema.registerFeature("shop_id", 1, 1);
  featuredb_schema.registerFeature("category1", 2, 1);
  featuredb_schema.registerFeature("category2", 3, 1);
  featuredb_schema.registerFeature("category3", 4, 1);

  cm::FeatureIndex feature_index(&featuredb_schema);

  /* set up cmdata */
  auto cmdata_path = flags.getString("cmdata");
  if (!FileUtil::isDirectory(cmdata_path)) {
    RAISEF(kIOError, "no such directory: $0", cmdata_path);
  }

  /* open featuredb */
  auto featuredb_path = FileUtil::joinPaths(
      cmdata_path,
      StringUtil::format("index/$0/db", cmcustomer));

  auto featuredb = mdb::MDB::open(featuredb_path, true);
  auto featuredb_txn = featuredb->startTransaction(true);

  /* open output file */
  auto outfile = File::openFile(
      outputfile_path,
      File::O_READ | File::O_WRITE | File::O_CREATE);

  /* mmap input file */
  io::MmappedFile mmap(File::openFile(inputfile_path, File::O_READ));
  madvise(mmap.data(), mmap.size(), MADV_WILLNEED);

  auto begin = (char *) mmap.begin();
  auto cur = begin;
  auto end = (char *) mmap.end();
  auto last = cur;
  uint64_t total_entries = 0;
  uint64_t total_bytes = 0;
  auto start_time = WallClock::now().unixMicros();
  auto last_status_line = start_time;

  /* status line */
  auto status_line = [
      begin,
      end,
      &cur,
      &total_bytes,
      &total_entries,
      &start_time,
      &last_status_line] {
    auto now = WallClock::now().unixMicros();
    if (now - last_status_line < 10000) {
      return;
    }

    last_status_line = now;
    auto runtime = (now - start_time) / 1000000;
    int percent = ((double) (cur - begin) / (double) (end - begin)) * 100;
    uint64_t bandwidth = total_bytes / (runtime + 1);

    auto str = StringUtil::format(
        "\r[$0%] scanning: entries=$1 bytes=$2B time=$3s bw=$4B/s" \
        "          ",
        percent,
        total_entries,
        total_bytes,
        runtime,
        bandwidth);

    write(0, str.c_str(), str.length());
    fflush(0);
  };

  HashMap<String, uint64_t> feature_counts;

  /* scan entries */
  for (; cur != end; ++cur) {
    status_line();

    if (*cur != separator) {
      continue;
    }

    if (cur > last + 1) {
      try {
        auto query = json::fromJSON<cm::JoinedQuery>(String(last, cur - last));

        for (const auto& item : query.items) {
          if (item.position > 16) {
            continue;
          }

          cm::Example ex;
          ex.label = item.clicked ? 1.0 : -1.0;
          ex.features = joinedQueryItemFeatures(
              query,
              item,
              &featuredb_schema,
              &feature_index,
              featuredb_txn.get());

          auto ex_str = exampleToSVMLight(ex, &output_feature_schema);
          if (ex_str.length() > 0) {
            outfile.write(ex_str + "\n");
          }
        }
      } catch (const Exception& e) {
        fnord::logError("cm.featuredump", e, "error");
      }

      total_bytes += cur - last;
      ++total_entries;
    }

    last = cur + 1;
  }

  status_line();
  write(0, "\n", 1);
  fflush(0);

  /* clean up */
  featuredb_txn->abort();
  exit(0);
}

