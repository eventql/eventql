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
#include "fnord-base/thread/eventloop.h"
#include "fnord-base/application.h"
#include "fnord-base/logging.h"
#include "fnord-base/Language.h"
#include "fnord-base/random.h"
#include "fnord-base/fnv.h"
#include "fnord-base/cli/flagparser.h"
#include "fnord-base/util/SimpleRateLimit.h"
#include "fnord-base/InternMap.h"
#include "fnord-base/thread/threadpool.h"
#include "fnord-json/json.h"
#include "fnord-mdb/MDB.h"
#include "fnord-mdb/MDBUtil.h"
#include "fnord-msg/msg.h"
#include "fnord-sstable/sstablereader.h"
#include "fnord-sstable/sstablewriter.h"
#include "fnord-sstable/SSTableColumnSchema.h"
#include "fnord-sstable/SSTableColumnReader.h"
#include "fnord-sstable/SSTableColumnWriter.h"
#include "fnord-http/httpconnectionpool.h"
#include <fnord-fts/fts.h>
#include <fnord-fts/fts_common.h>
#include <fnord-tsdb/TSDBClient.h>
#include "fnord-dproc/Application.h"
#include "fnord-dproc/LocalScheduler.h"
#include <fnord-dproc/Task.h>
#include "fnord-logtable/TableReader.h"
#include "common.h"
#include "schemas.h"
#include "CustomerNamespace.h"
#include "CTRCounter.h"
#include "analytics/ReportBuilder.h"
#include "analytics/AnalyticsTableScanSource.h"
#include "analytics/CTRByShopMapper.h"
#include "analytics/ECommerceStatsByShopMapper.h"
#include "analytics/ProductStatsByShopMapper.h"
#include "analytics/CTRCounterMergeReducer.h"
#include "analytics/CTRCounterTableSink.h"
#include "analytics/CTRCounterTableSource.h"
#include "analytics/RelatedTermsMapper.h"
#include "analytics/TopCategoriesByTermMapper.h"
#include "analytics/TermInfoMergeReducer.h"
#include "ItemBoost.pb.h"

using namespace fnord;
using namespace cm;

fnord::thread::EventLoop ev;

class ItemBoostMapper : public dproc::ProtoTask<ItemBoostMapperParams, ItemBoostResult> {
public:
  ItemBoostMapper(const ItemBoostMapperParams& params);
  void run(ItemBoostResult* result) override;
};

ItemBoostMapper::ItemBoostMapper(const ItemBoostMapperParams& params) {}

void ItemBoostMapper::run(ItemBoostResult* result) {

}

class ItemBoostReducer : public dproc::ProtoTask<ItemBoostReducerParams, ItemBoostResult> {
public:
  ItemBoostReducer(const ItemBoostReducerParams& params, tsdb::TSDBClient* tsdb);
  void run(ItemBoostResult* result) override;
  List<dproc::TaskDependency> dependencies() override;
protected:
  tsdb::TSDBClient* tsdb_;
};

ItemBoostReducer::ItemBoostReducer(
    const ItemBoostReducerParams& params,
    tsdb::TSDBClient* tsdb) :
    tsdb_(tsdb) {}

void ItemBoostReducer::run(ItemBoostResult* result) {

}

List<dproc::TaskDependency> ItemBoostReducer::dependencies() {
  auto input_partitions = tsdb_->listPartitions(
      "joined_sessions.dawanda",
      WallClock::unixMicros() - 90 * kMicrosPerDay,
      WallClock::unixMicros() - 6 * kMicrosPerHour);

  List<dproc::TaskDependency> deps;
  for (const auto& partition : input_partitions) {
    ItemBoostMapperParams dparams;
    dparams.set_stream_key("joined_sessions.dawanda");
    dparams.set_partition_key(partition);

    deps.emplace_back(dproc::TaskDependency {
      .task_name = "ItemBoostMapper",
      .params = *msg::encode(dparams)
    });
  }

  return deps;
}

int main(int argc, const char** argv) {
  fnord::Application::init();
  fnord::Application::logToStderr();

  fnord::cli::FlagParser flags;

  flags.defineFlag(
      "tempdir",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "artifact directory",
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

  /* start event loop */
  auto evloop_thread = std::thread([] {
    ev.run();
  });

  auto tempdir = flags.getString("tempdir");

  http::HTTPConnectionPool http(&ev);
  tsdb::TSDBClient tsdb("http://nue03.prod.fnrd.net:7003/tsdb", &http);

  dproc::Application app("cm.itemboost");
  app.registerProtoTask<ItemBoostMapper>("ItemBoostMapper");
  app.registerProtoTask<ItemBoostReducer>("ItemBoostReducer", &tsdb);

  dproc::LocalScheduler sched;
  sched.start();

  ItemBoostReducerParams params;
  params.set_customer("dawanda");
  params.set_from_unixmicros(WallClock::unixMicros() - 30 * kMicrosPerDay);
  params.set_until_unixmicros(WallClock::unixMicros() - 6 * kMicrosPerHour);

  auto res = sched.run(&app, "ItemBoostReducer", *msg::encode(params));

  sched.stop();
  ev.shutdown();
  evloop_thread.join();

  return 0;
}

