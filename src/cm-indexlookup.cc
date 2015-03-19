/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#define BOOST_NO_CXX11_NUMERIC_LIMITS 1
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "fnord-base/io/filerepository.h"
#include "fnord-base/io/fileutil.h"
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
#include "fnord-fts/fts.h"
#include "fnord-fts/fts_common.h"
#include "fnord-mdb/MDB.h"
#include "CustomerNamespace.h"
#include "FeatureSchema.h"
#include "FeaturePack.h"
#include "DocStore.h"
#include "FeatureIndex.h"
#include "IndexRequest.h"

using namespace cm;
using namespace fnord;

int main(int argc, const char** argv) {
  Application::init();
  Application::logToStderr();

  cli::FlagParser flags;

  flags.defineFlag(
      "index",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "index data dir",
      "<path>");

  flags.defineFlag(
      "docid",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "docid",
      "<docid>");

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

  /* arguments */
  DocID docid = { flags.getString("docid") };

  /* set up feature schema */
  FeatureSchema feature_schema;
  feature_schema.registerFeature("shop_id", 1, 1);
  feature_schema.registerFeature("category1", 2, 1);
  feature_schema.registerFeature("category2", 3, 1);
  feature_schema.registerFeature("category3", 4, 1);
  feature_schema.registerFeature("title~de", 5, 2);

  /* open featuredb db */
  auto featuredb_path = StringUtil::format("$0/db", flags.getString("index"));
  auto featuredb = mdb::MDB::open(featuredb_path, true);

  /* open full index  */
  auto fullindex_path = StringUtil::format("$0/docs", flags.getString("index"));
  cm::DocStore full_index(fullindex_path);

  /* get features */
  FeaturePack features;
  FeatureIndex feature_index(featuredb, &feature_schema);
  feature_index.getFeatures(docid, &features);

  /* get document */
  auto doc = full_index.findDocument(docid);

  /* dump document */
  doc->debugPrint();

  //fnord::iputs("\n[features]")
  //for (const auto& f : features) {
  //  auto feature_key = feature_schema.featureKey(f.first);
  //  fnord::iputs(
  //      "  $0=$1",
  //      feature_key.isEmpty() ? "unknown-feature": feature_key.get(),
  //      f.second);
  //}

  return 0;
}

