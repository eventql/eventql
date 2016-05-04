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
#include "CustomerNamespace.h"

#
#include "eventql/CTRCounter.h"

using namespace stx;

struct PosiInfo {
  PosiInfo() : views(0), clicks(0) {}
  uint64_t views;
  uint64_t clicks;
};

int main(int argc, const char** argv) {
  stx::Application::init();
  stx::Application::logToStderr();

  stx::cli::FlagParser flags;

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

  HashMap<uint32_t, PosiInfo> click_posis;

  auto start_time = std::numeric_limits<uint64_t>::max();
  auto end_time = std::numeric_limits<uint64_t>::min();
  auto eligibility = zbase::ItemEligibility::DAWANDA_ALL_NOBOTS;

  util::SimpleRateLimitedFn print_results(kMicrosPerSecond * 10, [&] () {
    uint64_t total_clicks = 0;
    uint64_t total_views = 0;
    Vector<Pair<uint64_t, PosiInfo>> posis;
    for (const auto& p : click_posis) {
      total_clicks += p.second.clicks;
      total_views += p.second.views;
      posis.emplace_back(p);
    }

    std::sort(posis.begin(), posis.end(), [] (
        const Pair<uint64_t, PosiInfo>& a,
        const Pair<uint64_t, PosiInfo>& b) {
      return a.first < b.first;
    });

    for (const auto& p : posis) {
      auto share = (p.second.clicks / (double) total_clicks) * 100;
      auto ctr = p.second.clicks / (double) p.second.views;

      stx::iputs("  position $0 => views=$1 clicks=$2 share=$3 ctr=$4",
          p.first,
          p.second.views,
          p.second.clicks,
          share,
          ctr);
    }
  });

  /* read input tables */
  auto sstables = flags.getArgv();
  int row_idx = 0;
  for (int tbl_idx = 0; tbl_idx < sstables.size(); ++tbl_idx) {
    const auto& sstable = sstables[tbl_idx];
    stx::logInfo("cm.ctrstats", "Importing sstable: $0", sstable);

    /* read sstable header */
    sstable::SSTableReader reader(File::openFile(sstable, File::O_READ));

    if (reader.bodySize() == 0) {
      stx::logCritical("cm.ctrstats", "unfinished sstable: $0", sstable);
      exit(1);
    }

    /* read report header */
    auto hdr = json::parseJSON(reader.readHeader());

    auto tbl_start_time = json::JSONUtil::objectGetUInt64(
        hdr.begin(),
        hdr.end(),
        "start_time").get();

    auto tbl_end_time = json::JSONUtil::objectGetUInt64(
        hdr.begin(),
        hdr.end(),
        "end_time").get();

    if (tbl_start_time < start_time) {
      start_time = tbl_start_time;
    }

    if (tbl_end_time > end_time) {
      end_time = tbl_end_time;
    }

    /* get sstable cursor */
    auto cursor = reader.getCursor();
    auto body_size = reader.bodySize();

    /* status line */
    util::SimpleRateLimitedFn status_line(kMicrosPerSecond, [&] () {
      stx::logInfo(
          "cm.ctrstats",
          "[$1/$2] [$0%] Reading sstable... rows=$3",
          (size_t) ((cursor->position() / (double) body_size) * 100),
          tbl_idx + 1, sstables.size(), row_idx);
    });

    /* read sstable rows */
    for (; cursor->valid(); ++row_idx) {
      status_line.runMaybe();
      print_results.runMaybe();

      auto val = cursor->getDataBuffer();
      Option<zbase::JoinedQuery> q;

      try {
        q = Some(json::fromJSON<zbase::JoinedQuery>(val));
      } catch (const Exception& e) {
        //stx::logWarning("cm.ctrstats", e, "invalid json: $0", val.toString());
      }

      if (!q.isEmpty() && isQueryEligible(eligibility, q.get())) {
        for (auto& item : q.get().items) {
          if (!isItemEligible(eligibility, q.get(), item) ||
              item.position < 1) {
            continue;
          }

          auto& pi = click_posis[item.position];
          ++pi.views;
          pi.clicks += item.clicked;
        }
      }

      if (!cursor->next()) {
        break;
      }
    }

    status_line.runForce();
  }

  print_results.runForce();
  return 0;
}

