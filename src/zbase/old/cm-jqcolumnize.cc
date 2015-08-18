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
#include <thread>
#include "stx/io/fileutil.h"
#include "stx/application.h"
#include "stx/logging.h"
#include "stx/cli/flagparser.h"
#include "stx/util/SimpleRateLimit.h"
#include "stx/InternMap.h"
#include "stx/thread/eventloop.h"
#include "stx/http/httpconnectionpool.h"
#include "stx/json/json.h"
#include "stx/mdb/MDB.h"
#include "stx/mdb/MDBUtil.h"
#include "sstable/sstablereader.h"
#include "sstable/SSTableEditor.h"
#include "sstable/SSTableColumnSchema.h"
#include "sstable/SSTableColumnReader.h"
#include "sstable/SSTableColumnWriter.h"
#include "cstable/BitPackedIntColumnReader.h"
#include "cstable/BitPackedIntColumnWriter.h"
#include "cstable/UInt32ColumnReader.h"
#include "cstable/UInt64ColumnReader.h"
#include "cstable/UInt32ColumnWriter.h"
#include "cstable/StringColumnWriter.h"
#include "cstable/BooleanColumnReader.h"
#include "cstable/BooleanColumnWriter.h"
#include "cstable/CSTableWriter.h"
#include "cstable/CSTableReader.h"
#include "cstable/CSTableBuilder.h"
#include "cstable/RecordMaterializer.h"
#include "stx/protobuf/MessageSchema.h"
#include "stx/protobuf/MessageBuilder.h"
#include "stx/protobuf/MessageObject.h"
#include "stx/protobuf/MessageEncoder.h"
#include "stx/protobuf/MessageDecoder.h"
#include "stx/protobuf/MessagePrinter.h"
#include <fnord-fts/fts.h>
#include <fnord-fts/fts_common.h>
#include "common.h"
#include "schemas.h"
#include "CustomerNamespace.h"
#include "zbase/CTRCounter.h"
#include "zbase/AnalyticsTableScan.h"
#include "zbase/CTRByPositionQuery.h"

using namespace zbase;
using namespace stx;

stx::thread::EventLoop ev;

int main(int argc, const char** argv) {
  stx::Application::init();
  stx::Application::logToStderr();

  stx::cli::FlagParser flags;

  flags.defineFlag(
      "file",
      stx::cli::FlagParser::T_STRING,
      true,
      "f",
      NULL,
      "file",
      "<filename>");

  flags.defineFlag(
      "upload_to",
      stx::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "url",
      "<url>");


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

  http::HTTPConnectionPool http(&ev);
  auto evloop_thread = std::thread([] {
    ev.run();
  });

  auto upload_to = flags.getString("upload_to");
  auto schema = joinedSessionsSchema();

  cstable::CSTableReader reader(flags.getString("file"));
  auto msgid_col_ref = reader.getColumnReader("__msgid");
  auto msgid_col = dynamic_cast<cstable::UInt64ColumnReader*>(msgid_col_ref.get());

  cstable::RecordMaterializer record_reader(&schema, &reader);

  util::BinaryMessageWriter batch;

  auto n = reader.numRecords();
  for (size_t i = 0; i < n; ) {
    for (; i < n;) {
      uint64_t msgid;
      uint64_t r;
      uint64_t d;
      msgid_col->next(&r, &d, &msgid);
      ++i;

      msg::MessageObject obj;
      record_reader.nextRecord(&obj);
      Buffer msg_buf;
      msg::MessageEncoder::encode(obj, schema, &msg_buf);
      uint64_t time;
      try {
        time = obj.getUInt64(schema.id("first_seen_time")) * kMicrosPerSecond;
      } catch (...) {
        stx::iputs("skipping row b/c it has no first_seen_time: $0", msgid);
        continue;
      }

      batch.appendUInt64(time);
      batch.appendUInt64(msgid);
      batch.appendVarUInt(msg_buf.size());
      batch.append(msg_buf.data(), msg_buf.size());

      if (batch.size() > 1024 * 1024 * 8) {
        break;
      }
    }

    stx::iputs("upload batch: $0 -- $1/$2", batch.size(), i, n);
    URI uri(upload_to + "/tsdb/insert_batch?stream=joined_sessions.dawanda");

    http::HTTPRequest req(http::HTTPMessage::M_POST, uri.pathAndQuery());
    req.addHeader("Host", uri.hostAndPort());
    req.addHeader("Content-Type", "application/fnord-msg");
    req.addBody(batch.data(), batch.size());
    auto res = http.executeRequest(req);
    res.wait();

    const auto& r = res.get();
    if (r.statusCode() != 201) {
      RAISEF(kRuntimeError, "received non-201 response: $0", r.body().toString());
    }

    batch.clear();
  }

  ev.shutdown();
  evloop_thread.join();
  return 0;
}

