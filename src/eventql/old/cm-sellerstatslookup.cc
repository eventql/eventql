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
#include "eventql/util/io/filerepository.h"
#include "eventql/util/io/fileutil.h"
#include "eventql/util/application.h"
#include "eventql/util/logging.h"
#include "eventql/util/random.h"
#include "eventql/util/thread/eventloop.h"
#include "eventql/util/thread/threadpool.h"
#include "eventql/util/wallclock.h"
#include "eventql/util/rpc/ServerGroup.h"
#include "eventql/util/rpc/RPC.h"
#include "eventql/util/rpc/RPCClient.h"
#include "eventql/util/cli/flagparser.h"
#include "eventql/util/json/json.h"
#include "eventql/util/json/jsonrpc.h"
#include "eventql/util/http/httprouter.h"
#include "eventql/util/http/httpserver.h"
#include "brokerd/FeedService.h"
#include "brokerd/RemoteFeedFactory.h"
#include "brokerd/RemoteFeedReader.h"
#include "eventql/util/stats/statsdagent.h"
#include "eventql/util/fts.h"
#include "eventql/util/fts_common.h"
#include "eventql/util/mdb/MDB.h"
#include "CustomerNamespace.h"


#include "IndexChangeRequest.h"
#include "FeatureIndex.h"
#include "sellerstats/ActivityLog.h"
#include "sellerstats/SellerStatsLookup.h"

using namespace zbase;
using namespace stx;

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
      stx::cli::FlagParser::T_STRING,
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

