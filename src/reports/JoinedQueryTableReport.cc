/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "reports/JoinedQueryTableReport.h"
#include "fnord-json/json.h"
#include "fnord-sstable/sstablereader.h"
#include "fnord-sstable/sstablewriter.h"
#include "fnord-sstable/SSTableColumnSchema.h"
#include "fnord-sstable/SSTableColumnReader.h"
#include "fnord-sstable/SSTableColumnWriter.h"

using namespace fnord;

namespace cm {

JoinedQueryTableReport::JoinedQueryTableReport(
    const Set<String>& sstable_filenames) :
    input_files_(sstable_filenames) {}

void JoinedQueryTableReport::addReport(RefPtr<Report> report) {
  children_.emplace_back(report);
}

void JoinedQueryTableReport::onEvent(ReportEventType type, void* ev) {
  switch (type) {
    case ReportEventType::BEGIN:
      for (const auto& cld : children_) {
        cld->onEvent(ReportEventType::BEGIN, nullptr);
      }

      readTables();
      return;

    case ReportEventType::END:
      for (const auto& cld : children_) {
        cld->onEvent(ReportEventType::END, nullptr);
      }

      return;

    default:
      RAISE(kRuntimeError, "unknown event type");

  }
}

void JoinedQueryTableReport::readTables() {
  int row_idx = 0;
  int tbl_idx = 0;
  for (const auto& sstable : input_files_) {
    fnord::logInfo("cm.ctrstats", "Importing sstable: $0", sstable);

    /* read sstable header */
    sstable::SSTableReader reader(File::openFile(sstable, File::O_READ));
    if (reader.bodySize() == 0) {
      RAISEF(kRuntimeError, "unfinished sstable: $0", sstable);
    }

    /* read report header */
    //auto hdr = json::parseJSON(reader.readHeader());

    //auto tbl_start_time = json::JSONUtil::objectGetUInt64(
    //    hdr.begin(),
    //    hdr.end(),
    //    "start_time").get();

    //auto tbl_end_time = json::JSONUtil::objectGetUInt64(
    //    hdr.begin(),
    //    hdr.end(),
    //    "end_time").get();

    //if (tbl_start_time < start_time) {
    //  start_time = tbl_start_time;
    //}

    //if (tbl_end_time > end_time) {
    //  end_time = tbl_end_time;
    //}

    /* get sstable cursor */
    auto cursor = reader.getCursor();
    auto body_size = reader.bodySize();

    /* status line */
    //util::SimpleRateLimitedFn status_line(kMicrosPerSecond, [&] () {
    //  fnord::logInfo(
    //      "cm.ctrstats",
    //      "[$1/$2] [$0%] Reading sstable... rows=$3",
    //      (size_t) ((cursor->position() / (double) body_size) * 100),
    //      tbl_idx + 1, sstables.size(), row_idx);
    //});

    /* read sstable rows */
    for (; cursor->valid(); ++row_idx) {
      auto val = cursor->getDataBuffer();
      Option<cm::JoinedQuery> q;

      try {
        q = Some(json::fromJSON<cm::JoinedQuery>(val));
      } catch (const Exception& e) {
        fnord::logWarning("cm.ctrstats", e, "invalid json: $0", val.toString());
      }

      if (!q.isEmpty()) {
        for (const auto& cld : children_) {
          cld->onEvent(ReportEventType::JOINED_QUERY, &q.get());
        }
      }

      if (!cursor->next()) {
        break;
      }
    }
  }
}

Set<String> JoinedQueryTableReport::inputFiles() {
  return input_files_;
}

Set<String> JoinedQueryTableReport::outputFiles() {
  Set<String> files;

  for (const auto& cld : children_) {
    for (const auto& f : cld->outputFiles()) {
      files.emplace(f);
    }
  }

  return files;
}

} // namespace cm

