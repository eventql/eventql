/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _libstx_HTTPSTATS_H
#define _libstx_HTTPSTATS_H

#include "stx/io/fileutil.h"
#include "stx/stdtypes.h"
#include "stx/stats/counter.h"
#include "stx/stats/multicounter.h"
#include "stx/stats/statsrepository.h"

namespace stx {
namespace http {

struct HTTPClientStats {
  stats::Counter<uint64_t> current_connections;
  stats::Counter<uint64_t> total_connections;
  stats::MultiCounter<uint64_t, uint64_t> status_codes;
  stats::Counter<uint64_t> current_requests;
  stats::Counter<uint64_t> total_requests;
  stats::Counter<uint64_t> received_bytes;
  stats::Counter<uint64_t> sent_bytes;

  HTTPClientStats() :
      status_codes(("http_status")) {}

  void exportStats(
      const String& path_prefix = "/fnord/http/client/",
      stats::StatsRepository* stats_repo = nullptr) {

    if (stats_repo == nullptr) {
      stats_repo = stats::StatsRepository::get();
    }

    stats_repo->exportStat(
        FileUtil::joinPaths(path_prefix, "current_connections"),
        &current_connections,
        stats::ExportMode::EXPORT_NONE);

    stats_repo->exportStat(
        FileUtil::joinPaths(path_prefix, "total_connections"),
        &total_connections,
        stats::ExportMode::EXPORT_DELTA);

    stats_repo->exportStat(
        FileUtil::joinPaths(path_prefix, "status_codes"),
        &status_codes,
        stats::ExportMode::EXPORT_DELTA);

    stats_repo->exportStat(
        FileUtil::joinPaths(path_prefix, "current_requests"),
        &current_requests,
        stats::ExportMode::EXPORT_NONE);

    stats_repo->exportStat(
        FileUtil::joinPaths(path_prefix, "total_requests"),
        &total_requests,
        stats::ExportMode::EXPORT_DELTA);

    stats_repo->exportStat(
        FileUtil::joinPaths(path_prefix, "received_bytes"),
        &received_bytes,
        stats::ExportMode::EXPORT_DELTA);

    stats_repo->exportStat(
        FileUtil::joinPaths(path_prefix, "sent_bytes"),
        &sent_bytes,
        stats::ExportMode::EXPORT_DELTA);
  }

};

struct HTTPServerStats {
  stats::Counter<uint64_t> current_connections;
  stats::Counter<uint64_t> total_connections;
  stats::MultiCounter<uint64_t, uint64_t> status_codes;
  stats::Counter<uint64_t> current_requests;
  stats::Counter<uint64_t> total_requests;
  stats::Counter<uint64_t> received_bytes;
  stats::Counter<uint64_t> sent_bytes;

  HTTPServerStats() :
      status_codes(("http_status")) {}

  void exportStats(
      const String& path_prefix = "/fnord/http/client/",
      stats::StatsRepository* stats_repo = nullptr) {

    if (stats_repo == nullptr) {
      stats_repo = stats::StatsRepository::get();
    }

    stats_repo->exportStat(
        FileUtil::joinPaths(path_prefix, "current_connections"),
        &current_connections,
        stats::ExportMode::EXPORT_NONE);

    stats_repo->exportStat(
        FileUtil::joinPaths(path_prefix, "total_connections"),
        &total_connections,
        stats::ExportMode::EXPORT_DELTA);

    stats_repo->exportStat(
        FileUtil::joinPaths(path_prefix, "status_codes"),
        &status_codes,
        stats::ExportMode::EXPORT_DELTA);

    stats_repo->exportStat(
        FileUtil::joinPaths(path_prefix, "current_requests"),
        &current_requests,
        stats::ExportMode::EXPORT_NONE);

    stats_repo->exportStat(
        FileUtil::joinPaths(path_prefix, "total_requests"),
        &total_requests,
        stats::ExportMode::EXPORT_DELTA);

    stats_repo->exportStat(
        FileUtil::joinPaths(path_prefix, "received_bytes"),
        &received_bytes,
        stats::ExportMode::EXPORT_DELTA);

    stats_repo->exportStat(
        FileUtil::joinPaths(path_prefix, "sent_bytes"),
        &sent_bytes,
        stats::ExportMode::EXPORT_DELTA);

  }
};

}
}
#endif
