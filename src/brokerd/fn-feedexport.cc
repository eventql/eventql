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
#include "stx/io/filerepository.h"
#include "stx/io/fileutil.h"
#include "stx/application.h"
#include "stx/logging.h"
#include "stx/random.h"
#include "stx/thread/eventloop.h"
#include "stx/thread/threadpool.h"
#include "stx/wallclock.h"
#include "stx/rpc/ServerGroup.h"
#include "stx/rpc/RPC.h"
#include "stx/rpc/RPCClient.h"
#include "stx/cli/flagparser.h"
#include "stx/json/json.h"
#include "stx/json/jsonrpc.h"
#include "stx/http/httprouter.h"
#include "stx/http/httpserver.h"
#include "brokerd/FeedService.h"
#include "brokerd/RemoteFeedFactory.h"
#include "brokerd/RemoteFeedReader.h"
#include "stx/stats/statsdagent.h"
#include "sstable/sstablereader.h"
#include "stx/mdb/MDB.h"
#include "CustomerNamespace.h"
#include "zbase/logjoin/LogJoin.h"

using namespace stx;

struct FeedChunkInfo {
  uint64_t generation;
  uint64_t generation_window;
  uint64_t start_time;
  uint64_t end_time;
  HashMap<String, String> start_offsets;

  template <typename T>
  static void reflect(T* meta) {
    meta->prop(&FeedChunkInfo::generation, 1, "generation", false);
    meta->prop(&FeedChunkInfo::generation_window, 2, "generation_window", false);
    meta->prop(&FeedChunkInfo::start_time, 3, "start_time", false);
    meta->prop(&FeedChunkInfo::end_time, 4, "end_time", false);
    meta->prop(&FeedChunkInfo::start_offsets, 5, "start_offsets", false);
  };
};

int main(int argc, const char** argv) {
  stx::Application::init();
  stx::Application::logToStderr();

  stx::cli::FlagParser flags;

  flags.defineFlag(
      "output_path",
      stx::cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "output file path",
      "<path>");

  flags.defineFlag(
      "output_prefix",
      stx::cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "output filename prefix",
      "<prefix>");

  flags.defineFlag(
      "batch_size",
      stx::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "2048",
      "batch_size",
      "<num>");

  flags.defineFlag(
      "buffer_size",
      stx::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "8192",
      "buffer_size",
      "<num>");

  flags.defineFlag(
      "generation_window_secs",
      stx::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "14400",
      "generation_window_secs",
      "<secs>");

  flags.defineFlag(
      "generation_delay_secs",
      stx::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "3600",
      "generation_delay_secs",
      "<secs>");

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

  /* start event loop */
  stx::thread::EventLoop ev;

  auto evloop_thread = std::thread([&ev] {
    ev.run();
  });

  /* set up rpc client */
  HTTPRPCClient rpc_client(&ev);

  auto output_path = flags.getString("output_path");
  auto output_prefix = flags.getString("output_prefix");
  size_t batch_size = flags.getInt("batch_size");
  size_t buffer_size = flags.getInt("buffer_size");
  size_t generation_window_micros =
      flags.getInt("generation_window_secs") * kMicrosPerSecond;
  size_t generation_delay_micros =
      flags.getInt("generation_delay_secs") * kMicrosPerSecond;

  FileUtil::mkdir_p(output_path);

  HashMap<String, uint64_t> feed_offsets;
  HashMap<uint64_t, std::unique_ptr<sstable::SSTableEditor>> generations;
  Set<uint64_t> finished_generations;
  uint64_t max_gen = 0;

  /* check old sstables */
  FileUtil::ls(output_path, [
      output_path,
      output_prefix,
      &max_gen,
      &feed_offsets,
      &finished_generations] (const String& filename) -> bool {
    if (!StringUtil::beginsWith(filename, output_prefix) ||
        !StringUtil::endsWith(filename, ".sstable")) {
      return true;
    }

    auto filepath = FileUtil::joinPaths(output_path, filename);
    sstable::SSTableReader reader(
        File::openFile(filepath, File::O_READ));

    if (reader.bodySize() == 0) {
        stx::logWarning(
            "fnord.feedexport",
            "Deleting unfinished sstable $0",
            filepath);

      FileUtil::rm(filepath);
      return true;
    }

    auto hdr = reader.readHeader();
    auto fci = json::fromJSON<FeedChunkInfo>(hdr);

    for (const auto& fo : fci.start_offsets) {
      auto foff = std::stoul(fo.second);
      if (feed_offsets[fo.first] < foff) {
        feed_offsets[fo.first] = foff;
      }
    }

    if (fci.generation > max_gen) {
      max_gen = fci.generation;
    }

    finished_generations.emplace(fci.generation);
    return true;
  });

  /* set up input feed reader */
  feeds::RemoteFeedReader feed_reader(&rpc_client);
  feed_reader.setMaxSpread(generation_delay_micros / 2);

  /* get source urls */
  Vector<String> uris = flags.getArgv();
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

    uint64_t offset = feed_offsets[feed];
    std::string offset_str;
    if (URI::getParam(params, "offset", &offset_str)) {
      if (offset_str == "HEAD") {
        offset = std::numeric_limits<uint64_t>::max();
      } else {
        offset = std::stoul(offset_str);
      }
    }

    feed_reader.addSourceFeed(
        uri,
        feed,
        offset,
        batch_size,
        buffer_size);
  }

  uint64_t total_rows = 0;
  uint64_t total_rows_written = 0;
  uint64_t total_bytes = 0;
  auto start_time = WallClock::now().unixMicros();
  auto last_status_line = start_time;

  UnixTime last_iter;
  uint64_t rate_limit_micros = 0.1 * kMicrosPerSecond;

  for (;;) {
    last_iter = WallClock::now();
    feed_reader.fillBuffers();

    /* read feed and build generations */
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
        for (const auto& g : generations) {
          active_gens.emplace(g.first);
        }

        auto watermarks = feed_reader.watermarks();
        auto runtime = (now - start_time) / 1000000;
        uint64_t bandwidth = total_bytes / (runtime + 1);
        auto str = StringUtil::format(
            "\rrows=$0 bytes=$1B time=$2s bw=$3B/s active_gens=$4 streamtime=$5"
            "          ",
            total_rows,
            total_bytes,
            runtime,
            bandwidth,
            inspect(active_gens),
            watermarks.first);

        write(0, str.c_str(), str.length());
        fflush(0);
      }

      auto entry_time = entry.get().time.unixMicros();
      if (entry_time == 0) {
        continue;
      }

      uint64_t entry_gen = entry_time / generation_window_micros;

      auto iter = generations.find(entry_gen);
      if (iter == generations.end()) {
        if (max_gen >= entry_gen) {
          if (finished_generations.count(entry_gen) == 0) {
            stx::logWarning(
                "fnord.feedexport",
                "Dropping late data for  generation #$0",
                entry_gen);
          }

          continue;
        }

        stx::logDebug(
            "fnord.feedexport",
            "Creating new generation #$0",
            entry_gen);

        FeedChunkInfo fci;
        fci.generation = entry_gen;
        fci.generation_window = generation_window_micros;
        fci.start_time = entry_gen * generation_window_micros;
        fci.end_time = (entry_gen + 1) * generation_window_micros;

        auto stream_offsets = feed_reader.streamOffsets();
        for (const auto& soff : stream_offsets) {
          fci.start_offsets.emplace(
              soff.first,
              StringUtil::toString(soff.second));
        }

        auto fci_json = json::toJSONString(fci);

        auto sstable_file_path = FileUtil::joinPaths(
            output_path,
            StringUtil::format("$0.$1.sstable", output_prefix, entry_gen));

        auto sstable_writer = sstable::SSTableEditor::create(
            sstable_file_path,
            sstable::IndexProvider{},
            fci_json.c_str(),
            fci_json.length());

        if (entry_gen > max_gen) {
          max_gen = entry_gen;
        }

        generations.emplace(entry_gen, std::move(sstable_writer));
      }

      const auto& entry_data = entry.get().data;
      generations[entry_gen]->appendRow(
          (void *) &entry_time,
          sizeof(entry_time),
          entry_data.c_str(),
          entry_data.length());

      ++total_rows_written;
    }

    feed_reader.fillBuffers();

    /* flush generations */
    auto watermarks = feed_reader.watermarks();
    auto stream_time_micros = watermarks.first.unixMicros();

    if (stream_time_micros > generation_delay_micros) {
      stream_time_micros -= generation_delay_micros;
    }

    for (auto iter = generations.begin(); iter != generations.end(); ) {
      if (stream_time_micros < ((iter->first + 1) * generation_window_micros)) {
        ++iter;
        continue;
      }

      stx::logDebug(
          "cm.ctrstatsbuild",
          "Flushing report generation #$0",
          iter->first);

      iter->second->finalize();
      iter = generations.erase(iter);
    }

    auto etime = WallClock::now().unixMicros() - last_iter.unixMicros();
    if (i < 1 && etime < rate_limit_micros) {
      usleep(rate_limit_micros - etime);
    }
  }

  evloop_thread.join();
  return 0;
}

