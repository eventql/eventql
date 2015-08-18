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
#include "stx/io/filerepository.h"
#include "stx/io/fileutil.h"
#include "stx/application.h"
#include "stx/logging.h"
#include "stx/random.h"
#include "stx/thread/eventloop.h"
#include "stx/thread/threadpool.h"
#include "stx/wallclock.h"
#include "stx/rpc/ServerGroup.h"
#include "stx/rpc/RPC.h"
#include "stx/rpc/RPCClient.h"
#include "stx/cli/flagparser.h"
#include "stx/json/json.h"
#include "stx/json/jsonrpc.h"
#include "stx/http/httprouter.h"
#include "stx/http/httpserver.h"
#include "brokerd/FeedService.h"
#include "brokerd/RemoteFeedFactory.h"
#include "brokerd/RemoteFeedReader.h"
#include "stx/stats/statsdagent.h"
#include "fnord-fts/fts.h"
#include "fnord-fts/fts_common.h"
#include "fnord-fts/FTSQuery.h"
#include "stx/mdb/MDB.h"
#include "CustomerNamespace.h"


#include "DocStore.h"
#include "FeatureIndex.h"
#include "IndexChangeRequest.h"
#include "IndexReader.h"

using namespace zbase;
using namespace stx;

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
      false,
      NULL,
      NULL,
      "docid",
      "<docid>");

  flags.defineFlag(
      "query",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "query",
      "<query>");

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

  auto index_path = flags.getString("index");

  auto index_reader = zbase::IndexReader::openIndex(index_path);
  stx::fts::Analyzer analyzer("./conf");

  if (flags.isSet("docid")) {
    DocID docid = { flags.getString("docid") };

    /* set up feature schema */
    FeatureSchema feature_schema;
    feature_schema.registerFeature("shop_id", 1, 1);
    feature_schema.registerFeature("category1", 2, 1);
    feature_schema.registerFeature("category2", 3, 1);
    feature_schema.registerFeature("category3", 4, 1);
    feature_schema.registerFeature("title~de", 5, 2);

    /* open featuredb db */
    auto featuredb_path = StringUtil::format("$0/db", index_path);
    auto featuredb = mdb::MDB::open(featuredb_path, true);

    /* open full index  */
    auto fullindex_path = StringUtil::format("$0/docs", index_path);
    zbase::DocStore full_index(fullindex_path);

    auto doc = full_index.findDocument(docid);
    doc->debugPrint();
    return 0;
  }

  if (flags.isSet("query")) {
    //stx::fts::FTSQuery fts_query;
    //fts_query.addField("title~de", 2.0);
    //fts_query.addField("text~de", 1.0);
    //fts_query.addQuery(flags.getString("query"), Language::DE, &analyzer);

    //auto searcher = std::make_shared<stx::fts::IndexSearcher>(
    //    index_reader->fts_);
    //fts_query.execute(searcher.get());
    //stx::iputs("found $0 documents", collector->getTotalHits());
    return 0;
  }

  //stx::iputs("\n[features]")
  //for (const auto& f : features) {
  //  auto feature_key = feature_schema.featureKey(f.first);
  //  stx::iputs(
  //      "  $0=$1",
  //      feature_key.isEmpty() ? "unknown-feature": feature_key.get(),
  //      f.second);
  //}

  return 0;
}

