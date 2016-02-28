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
#include "zbase/util/mdb/MDB.h"
#include "zbase/util/mdb/MDBUtil.h"
#include "sstable/sstablereader.h"
#include "sstable/SSTableEditor.h"
#include "sstable/SSTableColumnSchema.h"
#include "sstable/SSTableColumnReader.h"
#include "sstable/SSTableColumnWriter.h"
#include "common.h"
#include "CustomerNamespace.h"

#
#include "zbase/CTRCounter.h"

using namespace stx;

struct PageInfo {
  PageInfo() : views(0), clicks(0) {}
  uint64_t views;
  uint64_t clicks;
};


void indexJoinedQuery(
    const zbase::JoinedQuery& query,
    HashMap<uint32_t, PageInfo>* counters) {
  auto pg_str = zbase::extractAttr(query.attrs, "pg");
  if (pg_str.isEmpty()) {
    return;
  }

  auto pg = std::stoul(pg_str.get());

  bool any_clicked = false;
  for (auto& item : query.items) {
    if (item.clicked) {
      any_clicked = true;
      break;
    }
  }

  (*counters)[pg].views++;
  if (any_clicked) {
    (*counters)[pg].clicks++;
  }
}

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

  HashMap<uint32_t, PageInfo> click_posis;

  auto start_time = std::numeric_limits<uint64_t>::max();
  auto end_time = std::numeric_limits<uint64_t>::min();
  auto eligibility = zbase::ItemEligibility::DAWANDA_ALL_NOBOTS;


  util::SimpleRateLimitedFn print_results(kMicrosPerSecond * 10, [&] () {
    uint64_t total_clicks = 0;
    uint64_t total_views = 0;

    Vector<Pair<uint64_t, PageInfo>> pages;
    for (const auto& p : click_posis) {
      total_clicks += p.second.clicks;
      total_views += p.second.views;
      pages.emplace_back(p);
    }

    std::sort(pages.begin(), pages.end(), [] (
        const Pair<uint64_t, PageInfo>& a,
        const Pair<uint64_t, PageInfo>& b) {
      return a.first < b.first;
    });

    stx::iputs("page;views;clicks;ctr;viewshare;clickshare", 1);
    for (const auto& p : pages) {
      if (p.first > 100) {
        continue;
      }

      stx::iputs(
          "$0,$1,$2,$3,$4,$5",
          p.first,
          p.second.views,
          p.second.clicks,
          p.second.clicks / (double) p.second.views,
          p.second.views / (double) total_views,
          p.second.clicks / (double) total_clicks);
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
        stx::logWarning("cm.ctrstats", e, "invalid json: $0", val.toString());
      }

      if (!q.isEmpty()) {
        indexJoinedQuery(q.get(), &click_posis);
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

