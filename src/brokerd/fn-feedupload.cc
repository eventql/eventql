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
#include "stx/io/mmappedfile.h"
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
#include "stx/mdb/MDB.h"
#include "CustomerNamespace.h"
#include "zbase/logjoin/LogJoin.h"

using namespace stx;

int main(int argc, const char** argv) {
  stx::Application::init();
  stx::Application::logToStderr();

  stx::cli::FlagParser flags;

  flags.defineFlag(
      "input",
      stx::cli::FlagParser::T_STRING,
      true,
      "i",
      NULL,
      "input file",
      "<filename>");

  flags.defineFlag(
      "separator",
      stx::cli::FlagParser::T_STRING,
      false,
      "s",
      "\n",
      "separator",
      "<char>");

  flags.defineFlag(
      "concurrency",
      stx::cli::FlagParser::T_INTEGER,
      false,
      "c",
      "10",
      "concurrency",
      "<num>");

  flags.defineFlag(
      "loglevel",
      stx::cli::FlagParser::T_STRING,
      false,
      NULL,
      "CRITICAL",
      "loglevel",
      "<level>");

  flags.parseArgv(argc, argv);

  Logger::get()->setMinimumLogLevel(
      strToLogLevel(flags.getString("loglevel")));

  /* args */
  auto concurrency = flags.getInt("concurrency");
  auto inputfile = flags.getString("input");
  auto separator_str = flags.getString("separator");
  if (separator_str.length() != 1) {
    // print usage
    return 1;
  }

  char separator = separator_str[0];

  auto uris = flags.getArgv();
  if (uris.size() == 0) {
    // print usage...
    return 1;
  }

  /* start event loop */
  stx::thread::EventLoop ev;

  auto evloop_thread = std::thread([&ev] {
    ev.run();
  });

  /* set up rpc client */
  HTTPRPCClient rpc_client(&ev);

  /* set up feedwriter */
  feeds::RemoteFeedWriter feed_writer(&rpc_client);

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

    auto rpc_url = uri_raw.substr(0, uri_raw.find("?"));
    feed_writer.addTargetFeed(rpc_url, feed, concurrency);
  }

  /* mmap input file */
  io::MmappedFile mmap(File::openFile(inputfile, File::O_READ));
  auto begin = (char *) mmap.begin();
  auto cur = begin;
  auto end = (char *) mmap.end();
  auto last = cur;
  uint64_t total_entries = 0;
  uint64_t total_bytes = 0;
  auto start_time = WallClock::now().unixMicros();
  auto last_status_line = start_time;

  /* status line */
  auto status_line = [
      begin,
      end,
      &cur,
      &total_bytes,
      &total_entries,
      &start_time,
      &last_status_line,
      &feed_writer] {
    auto now = WallClock::now().unixMicros();
    if (now - last_status_line < 10000) {
      return;
    }

    last_status_line = now;
    auto runtime = (now - start_time) / 1000000;
    int percent = ((double) (cur - begin) / (double) (end - begin)) * 100;
    uint64_t bandwidth = total_bytes / (runtime + 1);
    auto queuelen = feed_writer.queueLength();

    auto str = StringUtil::format(
        "\r[$0%] uploading: entries=$1 bytes=$2B time=$3s bw=$4B/s queue=$5" \
        "          ",
        percent,
        total_entries,
        total_bytes,
        runtime,
        bandwidth,
        queuelen);

    write(0, str.c_str(), str.length());
    fflush(0);
  };

  /* upload entries */
  for (; cur != end; ++cur) {
    status_line();

    if (*cur != separator) {
      continue;
    }

    if (cur > last + 1) {
      while (feed_writer.queueLength() >= feed_writer.maxQueueLength() - 1) {
        status_line();
        usleep(1000);
      }

      feed_writer.appendEntry(String(last, cur - last));
      total_bytes += cur - last;
      ++total_entries;
    }

    last = cur + 1;
  }

  while (feed_writer.queueLength() > 0) {
    status_line();
    usleep(1000);
  }

  status_line();
  write(0, "\n", 1);
  fflush(0);

  ev.shutdown();
  evloop_thread.join();

  return 0;
}

