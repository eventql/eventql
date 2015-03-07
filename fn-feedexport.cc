/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include <unistd.h>
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
#include "fnord-mdb/MDB.h"
#include "cm-common/CustomerNamespace.h"
#include "cm-logjoin/LogJoin.h"

using namespace fnord;

int main(int argc, const char** argv) {
  fnord::Application::init();
  fnord::Application::logToStderr();

  fnord::cli::FlagParser flags;

  flags.defineFlag(
      "output_path",
      fnord::cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "output file path",
      "<path>");

  flags.defineFlag(
      "output_prefix",
      fnord::cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "output filename prefix",
      "<prefix>");

  flags.defineFlag(
      "batch_size",
      fnord::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "2048",
      "batch_size",
      "<num>");

  flags.defineFlag(
      "buffer_size",
      fnord::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "8192",
      "buffer_size",
      "<num>");

  flags.defineFlag(
      "generation_window_secs",
      fnord::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "14400",
      "generation_window_secs",
      "<secs>");

  flags.defineFlag(
      "generation_delay_secs",
      fnord::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "3600",
      "generation_delay_secs",
      "<secs>");

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
  fnord::thread::EventLoop ev;

  auto evloop_thread = std::thread([&ev] {
    ev.run();
  });

  /* set up rpc client */
  HTTPRPCClient rpc_client(&ev);

  size_t batch_size = flags.getInt("batch_size");
  size_t buffer_size = flags.getInt("buffer_size");
  size_t generation_window_micros =
      flags.getInt("generation_window_secs") * kMicrosPerSecond;
  size_t generation_delay_micros =
      flags.getInt("generation_delay_secs") * kMicrosPerSecond;

  /* set up input feed reader */
  feeds::RemoteFeedReader feed_reader(&rpc_client);

  /* get source urls */
  Vector<String> uris = flags.getArgv();
  //if (flags.isSet("statefile")) {
  //  auto statefile = FileUtil::read(flags.getString("statefile")).toString();
  //  for (const auto& uri : StringUtil::split(statefile, "\n")) {
  //    if (uri.size() > 0) {
  //      uris.emplace_back(uri);
  //    }
  //  }
  //}

  HashMap<String, String> feed_urls;
  for (const auto& uri_raw : uris) {
    URI uri(uri_raw);
    const auto& params = uri.queryParams();

    std::string feed;
    if (!URI::getParam(params, "feed", &feed)) {
      RAISEF(
          kIllegalArgumentError,
          "feed url missing ?feed query param: $0",
          uri_raw);
    }

    feed_urls.emplace(feed, uri_raw.substr(0, uri_raw.find("?")));

    //std::string offset_str;
    uint64_t offset = 0;
    //if (URI::getParam(params, "offset", &offset_str)) {
    //  if (offset_str == "HEAD") {
    //    offset = std::numeric_limits<uint64_t>::max();
    //  } else {
    //    offset = std::stoul(offset_str);
    //  }
    //}

    feed_reader.addSourceFeed(
        uri,
        feed,
        offset,
        batch_size,
        buffer_size);
  }

  uint64_t total_rows = 0;
  uint64_t total_bytes = 0;
  auto start_time = WallClock::now().unixMicros();
  auto last_status_line = start_time;

  DateTime last_iter;
  uint64_t rate_limit_micros = 0.1 * kMicrosPerSecond;

  HashMap<uint64_t, List<feeds::FeedEntry>> generations_;
  uint64_t max_gen_;

  for (;;) {
    last_iter = WallClock::now();
    feed_reader.fillBuffers();

    int i = 0;
    for (; i < batch_size; ++i) {
      auto entry = feed_reader.fetchNextEntry();

      if (entry.isEmpty()) {
        break;
      }

      ++total_rows;
      total_bytes += entry.get().data.size();

      auto now = WallClock::now().unixMicros();
      if (now - last_status_line > 10000) {
        Set<uint64_t> active_gens;
        for (const auto& g : generations_) {
          active_gens.emplace(g.first);
        }

        auto runtime = (now - start_time) / 1000000;
        uint64_t bandwidth = total_bytes / (runtime + 1);
        auto str = StringUtil::format(
            "\rrows=$0 bytes=$1B time=$2s bw=$3B/s active_gens=$4"
            "          ",
            total_rows,
            total_bytes,
            runtime,
            bandwidth,
            inspect(active_gens));

        write(0, str.c_str(), str.length());
        fflush(0);
      }


      auto entry_time = entry.get().time.unixMicros();
      if (entry_time == 0) {
        continue;
      }

      uint64_t entry_gen = entry_time / generation_window_micros;

      auto iter = generations_.find(entry_gen);
      if (iter == generations_.end()) {
        if (max_gen_ >= entry_gen) {
          fnord::logWarning(
              "fnord.feedexport",
              "Dropping late data for  generation #$0",
              entry_gen);

          continue;
        }

        fnord::logDebug(
            "fnord.feedexport",
            "Creating new generation #$0",
            entry_gen);
      }

      generations_[entry_gen].emplace_back(entry.get());
    }

    feed_reader.fillBuffers();

    auto etime = WallClock::now().unixMicros() - last_iter.unixMicros();
    if (i < 1 && etime < rate_limit_micros) {
      usleep(rate_limit_micros - etime);
    }
  }

  evloop_thread.join();
  return 0;
}

