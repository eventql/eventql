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
#include "IndexRequest.h"
#include "index/IndexBuilder.h"
#include "sellerstats/ActivityLog.h"
#include "sellerstats/SellerStatsLookup.h"

using namespace cm;
using namespace fnord;

int main(int argc, const char** argv) {
  Application::init();
  Application::logToStderr();

  cli::FlagParser flags;

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
      "shopid",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "shopid",
      "<shopid>");

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
  auto cmcustomer = flags.getString("cmcustomer");
  auto shopid = flags.getString("shopid");

  /* set up cmdata */
  auto cmdata_path = flags.getString("cmdata");
  if (!FileUtil::isDirectory(cmdata_path)) {
    RAISEF(kIOError, "no such directory: $0", cmdata_path);
  }

  /* lookup and print */
  auto lookup = SellerStatsLookup::lookup(shopid, cmdata_path, cmcustomer);
  write(0, lookup.c_str(), lookup.length());
  fflush(0);

  return 0;
}

