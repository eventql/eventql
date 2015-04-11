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
#include "fnord-msg/MessageSchema.h"
#include "fnord-msg/MessageBuilder.h"
#include "fnord-msg/MessageObject.h"
#include <fnord-fts/fts.h>
#include <fnord-fts/fts_common.h>
#include "common.h"
#include "CustomerNamespace.h"
#include "FeatureSchema.h"
#include "JoinedQuery.h"
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
      "conf",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      "./conf",
      "conf directory",
      "<path>");

  flags.defineFlag(
      "input_file",
      fnord::cli::FlagParser::T_STRING,
      true,
      "i",
      NULL,
      "file",
      "<filename>");

  flags.defineFlag(
      "output_file",
      fnord::cli::FlagParser::T_STRING,
      true,
      "o",
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

  Vector<msg::MessageSchemaField> fields;

  msg::MessageSchemaField queries(
      16,
      "queries",
      msg::FieldType::OBJECT,
      0,
      true,
      false);

  queries.fields.emplace_back(
      18,
      "time",
      msg::FieldType::UINT32,
      0xffffffff,
      false,
      false);

  queries.fields.emplace_back(
      1,
      "page",
      msg::FieldType::UINT32,
      100,
      false,
      true,
      msg::EncodingHint::BITPACK);

  queries.fields.emplace_back(
      2,
      "language",
      msg::FieldType::UINT32,
      kMaxLanguage,
      false,
      false,
      msg::EncodingHint::BITPACK);

  queries.fields.emplace_back(
      3,
      "query_string",
      msg::FieldType::STRING,
      8192,
      false,
      true);

  queries.fields.emplace_back(
      4,
      "query_string_normalized",
      msg::FieldType::STRING,
      8192,
      false,
      true);

  queries.fields.emplace_back(
      5,
      "num_items",
      msg::FieldType::UINT32,
      250,
      false,
      false,
      msg::EncodingHint::BITPACK);

  queries.fields.emplace_back(
      6,
      "num_items_clicked",
      msg::FieldType::UINT32,
      250,
      false,
      false,
      msg::EncodingHint::BITPACK);

  queries.fields.emplace_back(
      7,
      "num_ad_impressions",
      msg::FieldType::UINT32,
      250,
      false,
      false,
      msg::EncodingHint::BITPACK);

  queries.fields.emplace_back(
      8,
      "num_ad_clicks",
      msg::FieldType::UINT32,
      250,
      false,
      false,
      msg::EncodingHint::BITPACK);

  queries.fields.emplace_back(
      9,
      "ab_test_group",
      msg::FieldType::UINT32,
      100,
      false,
      true,
      msg::EncodingHint::BITPACK);

  queries.fields.emplace_back(
      10,
      "device_type",
      msg::FieldType::UINT32,
      kMaxDeviceType,
      false,
      false,
      msg::EncodingHint::BITPACK);

  queries.fields.emplace_back(
      11,
      "page_type",
      msg::FieldType::UINT32,
      kMaxPageType,
      false,
      false,
      msg::EncodingHint::BITPACK);

  queries.fields.emplace_back(
      12,
      "category1",
      msg::FieldType::UINT32,
      0xffff,
      false,
      true,
      msg::EncodingHint::BITPACK);

  queries.fields.emplace_back(
      13,
      "category2",
      msg::FieldType::UINT32,
      0xffff,
      false,
      true,
      msg::EncodingHint::BITPACK);

  queries.fields.emplace_back(
      14,
      "category3",
      msg::FieldType::UINT32,
      0xffff,
      false,
      true,
      msg::EncodingHint::BITPACK);

  msg::MessageSchemaField query_items(
      17,
      "items",
      msg::FieldType::OBJECT,
      0,
      true,
      false);

  query_items.fields.emplace_back(
      14,
      "position",
      msg::FieldType::UINT32,
      64,
      false,
      false,
      msg::EncodingHint::BITPACK);

  query_items.fields.emplace_back(
      15,
      "clicked",
      msg::FieldType::BOOLEAN,
      0,
      false,
      false);

  queries.fields.emplace_back(query_items);
  fields.emplace_back(queries);

  msg::MessageSchema schema("joined_session", fields);
  fnord::iputs("$0", schema.toString());

  fnord::fts::Analyzer analyzer(flags.getString("conf"));

  fnord::cstable::CSTableBuilder table(&schema);
  auto add_session = [&] (const cm::JoinedSession& sess) {
    msg::MessageObject obj;

    for (const auto& q : sess.queries) {
      auto& qry_obj = obj.addChild(schema.id("queries"));

      /* queries.time */
      qry_obj.addChild(
          schema.id("queries.time"),
          (uint32_t) (q.time.unixMicros() / kMicrosPerSecond));

      /* queries.language */
      auto lang = cm::extractLanguage(q.attrs);
      qry_obj.addChild(schema.id("queries.language"), (uint32_t) lang);

      /* queries.query_string */
      auto qstr = cm::extractQueryString(q.attrs);
      if (!qstr.isEmpty()) {
        auto qstr_norm = analyzer.normalize(lang, qstr.get());
        qry_obj.addChild(schema.id("queries.query_string"), qstr.get());
        qry_obj.addChild(schema.id("queries.query_string_normalized"), qstr_norm);
      }

      /* queries.num_item_clicks, queries.num_items */
      uint32_t nitems = 0;
      uint32_t nclicks = 0;
      uint32_t nads = 0;
      uint32_t nadclicks = 0;
      for (const auto& i : q.items) {
        // DAWANDA HACK
        if (i.position >= 1 && i.position <= 4) {
          ++nads;
          nadclicks += i.clicked;
        }
        // EOF DAWANDA HACK

        ++nitems;
        nclicks += i.clicked;
      }

      qry_obj.addChild(schema.id("queries.num_items"), nitems);
      qry_obj.addChild(schema.id("queries.num_items_clicked"), nclicks);
      qry_obj.addChild(schema.id("queries.num_ad_impressions"), nads);
      qry_obj.addChild(schema.id("queries.num_ad_clicks"), nadclicks);

      /* queries.page */
      auto pg_str = cm::extractAttr(q.attrs, "pg");
      if (!pg_str.isEmpty()) {
        uint32_t pg = std::stoul(pg_str.get());
        qry_obj.addChild(schema.id("queries.page"), pg);
      }

      /* queries.ab_test_group */
      auto abgrp = cm::extractABTestGroup(q.attrs);
      if (!abgrp.isEmpty()) {
        qry_obj.addChild(schema.id("queries.ab_test_group"), abgrp.get());
      }

      /* queries.category1 */
      auto qcat1 = cm::extractAttr(q.attrs, "q_cat1");
      if (!qcat1.isEmpty()) {
        uint32_t c = std::stoul(qcat1.get());
        qry_obj.addChild(schema.id("queries.category1"), c);
      }

      /* queries.category1 */
      auto qcat2 = cm::extractAttr(q.attrs, "q_cat2");
      if (!qcat2.isEmpty()) {
        uint32_t c = std::stoul(qcat2.get());
        qry_obj.addChild(schema.id("queries.category2"), c);
      }

      /* queries.category1 */
      auto qcat3 = cm::extractAttr(q.attrs, "q_cat3");
      if (!qcat3.isEmpty()) {
        uint32_t c = std::stoul(qcat3.get());
        qry_obj.addChild(schema.id("queries.category3"), c);
      }

      /* queries.device_type */
      qry_obj.addChild(
          schema.id("queries.device_type"),
          (uint32_t) extractDeviceType(q.attrs));

      /* queries.page_type */
      qry_obj.addChild(
          schema.id("queries.page_type"),
          (uint32_t) extractPageType(q.attrs));

      for (const auto& item : q.items) {
        auto& item_obj = qry_obj.addChild(schema.id("queries.items"));
        item_obj.addChild(
            schema.id("queries.items.position"),
            (uint32_t) item.position);
        item_obj.addChild(schema.id("queries.items.clicked"), item.clicked);
      }
    }

    table.addRecord(obj);
  };

  /* read input tables */
  int row_idx = 0;

  const auto& sstable = flags.getString("input_file");
  fnord::logInfo("cm.jqcolumnize", "Importing sstable: $0", sstable);

  /* read sstable header */
  sstable::SSTableReader reader(File::openFile(sstable, File::O_READ));

  if (reader.bodySize() == 0) {
    fnord::logCritical("cm.jqcolumnize", "unfinished sstable: $0", sstable);
    exit(1);
  }

  /* get sstable cursor */
  auto cursor = reader.getCursor();
  auto body_size = reader.bodySize();

  /* status line */
  util::SimpleRateLimitedFn status_line(kMicrosPerSecond, [&] () {
    fnord::logInfo(
        "cm.jqcolumnize",
        "[$0%] Reading sstable... rows=$1",
        (size_t) ((cursor->position() / (double) body_size) * 100),
        row_idx);
  });

  /* read sstable rows */
  for (; cursor->valid(); ++row_idx) {
    status_line.runMaybe();

    auto key = cursor->getKeyString();
    auto val = cursor->getDataBuffer();
    Option<cm::JoinedQuery> q;

    try {
      q = Some(json::fromJSON<cm::JoinedQuery>(val));
    } catch (const Exception& e) {
      fnord::logWarning("cm.jqcolumnize", e, "invalid json: $0", val.toString());
    }

    if (!q.isEmpty()) {
      cm::JoinedSession s;
      s.queries.emplace_back(q.get());
      add_session(s);
    }

    if (!cursor->next()) {
      break;
    }
  }

  status_line.runForce();

  table.write(flags.getString("output_file"));

  //{
  //  cstable::CSTableReader reader(flags.getString("output_file"));
  //  auto t0 = WallClock::unixMicros();

  //  cm::AnalyticsTableScan aq;
  //  auto lcol = aq.fetchColumn("queries.language");
  //  auto ccol = aq.fetchColumn("queries.page");
  //  auto qcol = aq.fetchColumn("queries.query_string_normalized");

  //  aq.onQuery([&] () {
  //    auto l = languageToString((Language) lcol->getUInt32());
  //    auto c = ccol->getUInt32();
  //    auto q = qcol->getString();
  //    fnord::iputs("lang: $0 -> $1 -- $2", l, c, q);
  //  });

  //  aq.scanTable(&reader);
  //}

  return 0;
}

