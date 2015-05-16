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
#include "analytics/AnalyticsTableScanReducer.h"
#include "analytics/AnalyticsTableScanMapper.h"
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


class ItemBoostScanlet : public AnalyticsTableScanlet<ItemBoostParams, ItemBoostResult> {
public:

  ItemBoostScanlet(const ItemBoostParams& params);

  void onQueryItem();
  void getResult(ItemBoostResult* result) override;

  static String name() { return "ItemBoost"; }
  static void mergeResult(ItemBoostResult* target, ItemBoostResult* other);

protected:
  RefPtr<AnalyticsTableScan::ColumnRef> itemid_col_;
  RefPtr<AnalyticsTableScan::ColumnRef> clicked_col_;
  HashMap<String, CTRCounterData> counters_;
};

ItemBoostScanlet::ItemBoostScanlet(
  const ItemBoostParams& params) :
  itemid_col_(scan_.fetchColumn("search_queries.result_items.item_id")),
  clicked_col_(scan_.fetchColumn("search_queries.result_items.clicked")) {
  scan_.onQueryItem(std::bind(&ItemBoostScanlet::onQueryItem, this));
}

void ItemBoostScanlet::onQueryItem() {
  auto itemid = itemid_col_->getString();
  auto clicked = clicked_col_->getBool();

  auto& ctr = counters_[itemid];
  ++ctr.num_views;
  if (clicked) ++ctr.num_clicks;
}

void ItemBoostScanlet::getResult(ItemBoostResult* result) {
  for (const auto& ctr : counters_) {
    auto ires = result->add_items();
    ires->set_item_id(ctr.first);
    ires->set_num_impressions(ctr.second.num_views);
    ires->set_num_clicks(ctr.second.num_clicks);
  }
}

fnord::thread::EventLoop ev;

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
  DistAnalyticsTableScan<ItemBoostScanlet>::registerWithApp(&app, &tsdb);

  dproc::LocalScheduler sched;
  sched.start();

  auto taskspec = DistAnalyticsTableScan<ItemBoostScanlet>::getTaskSpec(
      "dawanda",
      WallClock::unixMicros() - 30 * kMicrosPerDay,
      WallClock::unixMicros() - 6 * kMicrosPerHour,
      ItemBoostParams {});

  auto res = sched.run(&app, taskspec);

  fnord::iputs("exit...", 1);
  sched.stop();
  ev.shutdown();
  evloop_thread.join();
  exit(0);
}

