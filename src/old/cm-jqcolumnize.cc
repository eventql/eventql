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
#include "fnord/io/fileutil.h"
#include "fnord/application.h"
#include "fnord/logging.h"
#include "fnord/cli/flagparser.h"
#include "fnord/util/SimpleRateLimit.h"
#include "fnord/InternMap.h"
#include "fnord/thread/eventloop.h"
#include "fnord/http/httpconnectionpool.h"
#include "fnord/json/json.h"
#include "fnord/mdb/MDB.h"
#include "fnord/mdb/MDBUtil.h"
#include "sstable/sstablereader.h"
#include "sstable/sstablewriter.h"
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
#include "fnord/protobuf/MessageSchema.h"
#include "fnord/protobuf/MessageBuilder.h"
#include "fnord/protobuf/MessageObject.h"
#include "fnord/protobuf/MessageEncoder.h"
#include "fnord/protobuf/MessageDecoder.h"
#include "fnord/protobuf/MessagePrinter.h"
#include <fnord-fts/fts.h>
#include <fnord-fts/fts_common.h>
#include "common.h"
#include "schemas.h"
#include "CustomerNamespace.h"
#include "analytics/CTRCounter.h"
#include "analytics/AnalyticsTableScan.h"
#include "analytics/CTRByPositionQuery.h"

using namespace cm;
using namespace fnord;

fnord::thread::EventLoop ev;

int main(int argc, const char** argv) {
  fnord::Application::init();
  fnord::Application::logToStderr();

  fnord::cli::FlagParser flags;

  flags.defineFlag(
      "file",
      fnord::cli::FlagParser::T_STRING,
      true,
      "f",
      NULL,
      "file",
      "<filename>");

  flags.defineFlag(
      "upload_to",
      fnord::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "url",
      "<url>");


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
        fnord::iputs("skipping row b/c it has no first_seen_time: $0", msgid);
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

    fnord::iputs("upload batch: $0 -- $1/$2", batch.size(), i, n);
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

