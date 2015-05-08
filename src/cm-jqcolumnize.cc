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
#include "fnord-cstable/BitPackedIntColumnReader.h"
#include "fnord-cstable/BitPackedIntColumnWriter.h"
#include "fnord-cstable/UInt32ColumnReader.h"
#include "fnord-cstable/UInt32ColumnWriter.h"
#include "fnord-cstable/StringColumnWriter.h"
#include "fnord-cstable/BooleanColumnReader.h"
#include "fnord-cstable/BooleanColumnWriter.h"
#include "fnord-cstable/CSTableWriter.h"
#include "fnord-cstable/CSTableReader.h"
#include "fnord-cstable/CSTableBuilder.h"
#include "fnord-cstable/RecordMaterializer.h"
#include "fnord-msg/MessageSchema.h"
#include "fnord-msg/MessageBuilder.h"
#include "fnord-msg/MessageObject.h"
#include "fnord-msg/MessageEncoder.h"
#include "fnord-msg/MessageDecoder.h"
#include "fnord-msg/MessagePrinter.h"
#include <fnord-fts/fts.h>
#include <fnord-fts/fts_common.h>
#include "common.h"
#include "schemas.h"
#include "CustomerNamespace.h"
#include "CTRCounter.h"
#include "analytics/AnalyticsTableScan.h"
#include "analytics/CTRByPositionQuery.h"

using namespace cm;
using namespace fnord;

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

  auto schema = joinedSessionsSchema();

  cstable::CSTableReader reader(flags.getString("file"));

  fnord::iputs("has col: $0", reader.hasColumn("referrer_campaign"));
  exit(0);
  cstable::RecordMaterializer record_reader(&schema, &reader);

  for (int i = 0; i < reader.numRecords(); ++i) {
    msg::MessageObject obj;
    record_reader.nextRecord(&obj);
    fnord::iputs("record: $0", msg::MessagePrinter::print(obj, schema));
  }
  //cm::AnalyticsTableScan aq;
  //auto lcol = aq.fetchColumn("search_queries.language");
  //auto ccol = aq.fetchColumn("search_queries.page");
  //auto qcol = aq.fetchColumn("search_queries.query_string_normalized");
  //auto iicol = aq.fetchColumn("search_queries.result_items.item_id");
  //auto iscol = aq.fetchColumn("search_queries.result_items.shop_id");
  //auto ic1col = aq.fetchColumn("search_queries.result_items.category1");
  //auto ic2col = aq.fetchColumn("search_queries.result_items.category2");
  //auto ic3col = aq.fetchColumn("search_queries.result_items.category3");

  //aq.onQuery([&] () {
  //  auto l = languageToString((Language) lcol->getUInt32());
  //  auto c = ccol->getUInt32();
  //  auto q = qcol->getString();
  //  auto ii = iicol->getString();
  //  auto is = iscol->getUInt32();
  //  fnord::iputs("lang: $0 -> $1 -- $2 -- $3 -- $4 -- $5,$6,$7",
  //      l, c, q, ii, is,
  //      ic1col->getUInt32(),
  //      ic2col->getUInt32(),
  //      ic3col->getUInt32());
  //});

  //aq.scanTable(&reader);

  return 0;
}

