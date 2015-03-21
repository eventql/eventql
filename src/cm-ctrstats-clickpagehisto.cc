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
#include "fnord-base/application.h"
#include "fnord-base/logging.h"
#include "fnord-base/cli/flagparser.h"
#include "fnord-base/util/SimpleRateLimit.h"
#include "fnord-base/InternMap.h"
#include "fnord-json/json.h"
#include "fnord-mdb/MDB.h"
#include "fnord-mdb/MDBUtil.h"
#include "fnord-sstable/sstablereader.h"
#include "fnord-sstable/sstablewriter.h"
#include "fnord-sstable/SSTableColumnSchema.h"
#include "fnord-sstable/SSTableColumnReader.h"
#include "fnord-sstable/SSTableColumnWriter.h"
#include "common.h"
#include "CustomerNamespace.h"
#include "FeatureSchema.h"
#include "JoinedQuery.h"
#include "CTRCounter.h"

using namespace fnord;

struct PageInfo {
  PageInfo() : views(0), clicks(0) {}
  uint64_t views;
  uint64_t clicks;
};


void indexJoinedQuery(
    const cm::JoinedQuery& query,
    HashMap<uint32_t, PageInfo>* counters) {
  auto pg_str = cm::extractAttr(query.attrs, "pg");
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
  fnord::Application::init();
  fnord::Application::logToStderr();

  fnord::cli::FlagParser flags;

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

  HashMap<uint32_t, PageInfo> click_posis;

  auto start_time = std::numeric_limits<uint64_t>::max();
  auto end_time = std::numeric_limits<uint64_t>::min();
  auto eligibility = cm::ItemEligibility::DAWANDA_ALL_NOBOTS;


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

    fnord::iputs("page;views;clicks;ctr;viewshare;clickshare", 1);
    for (const auto& p : pages) {
      if (p.first > 100) {
        continue;
      }

      fnord::iputs(
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
    fnord::logInfo("cm.ctrstats", "Importing sstable: $0", sstable);

    /* read sstable header */
    sstable::SSTableReader reader(File::openFile(sstable, File::O_READ));

    if (reader.bodySize() == 0) {
      fnord::logCritical("cm.ctrstats", "unfinished sstable: $0", sstable);
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
      fnord::logInfo(
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
      Option<cm::JoinedQuery> q;

      try {
        q = Some(json::fromJSON<cm::JoinedQuery>(val));
      } catch (const Exception& e) {
        fnord::logWarning("cm.ctrstats", e, "invalid json: $0", val.toString());
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

