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
#include "fnord-msg/MessageSchema.h"
#include "fnord-msg/MessageBuilder.h"
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
      1,
      "page",
      msg::FieldType::UINT32,
      100,
      false,
      true);

  queries.fields.emplace_back(
      2,
      "language",
      msg::FieldType::UINT32,
      kMaxLanguage,
      false,
      false);

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
      false);

  queries.fields.emplace_back(
      6,
      "num_items_clicked",
      msg::FieldType::UINT32,
      250,
      false,
      false);

  queries.fields.emplace_back(
      7,
      "num_ad_impressions",
      msg::FieldType::UINT32,
      250,
      false,
      false);

  queries.fields.emplace_back(
      8,
      "num_ad_clicks",
      msg::FieldType::UINT32,
      250,
      false,
      false);

  queries.fields.emplace_back(
      9,
      "ab_test_group",
      msg::FieldType::UINT32,
      100,
      false,
      true);

  queries.fields.emplace_back(
      10,
      "device_type",
      msg::FieldType::UINT32,
      kMaxDeviceType,
      false,
      false);

  queries.fields.emplace_back(
      11,
      "page_type",
      msg::FieldType::UINT32,
      kMaxPageType,
      false,
      false);

  queries.fields.emplace_back(
      12,
      "category1",
      msg::FieldType::UINT32,
      0xffff,
      false,
      true);

  queries.fields.emplace_back(
      13,
      "category2",
      msg::FieldType::UINT32,
      0xffff,
      false,
      true);

  queries.fields.emplace_back(
      14,
      "category3",
      msg::FieldType::UINT32,
      0xffff,
      false,
      true);

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
      false);

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

  /* query level */
  //cstable::UInt32ColumnWriter jq_time_col(1, 1);
  //cstable::BitPackedIntColumnWriter jq_page_col(1, 1, 100);
  //cstable::BitPackedIntColumnWriter jq_lang_col(1, 1, kMaxLanguage);
  //cstable::StringColumnWriter jq_qstr_col(1, 1, 8192);
  //cstable::StringColumnWriter jq_qstrnorm_col(1, 1, 8192);
  //cstable::BitPackedIntColumnWriter jq_numitems_col(1, 1, 250);
  //cstable::BitPackedIntColumnWriter jq_numitemclicks_col(1, 1, 250);
  //cstable::BitPackedIntColumnWriter jq_numadimprs_col(1, 1, 250);
  //cstable::BitPackedIntColumnWriter jq_numadclicks_col(1, 1, 250);
  //cstable::BitPackedIntColumnWriter jq_abtestgroup_col(1, 1, 100);
  //cstable::BitPackedIntColumnWriter jq_devicetype_col(1, 1, kMaxDeviceType);
  //cstable::BitPackedIntColumnWriter jq_pagetype_col(1, 1, kMaxPageType);
  //cstable::BitPackedIntColumnWriter jq_cat1_col(1, 1, 0xffff);
  //cstable::BitPackedIntColumnWriter jq_cat2_col(1, 1, 0xffff);
  //cstable::BitPackedIntColumnWriter jq_cat3_col(1, 1, 0xffff);

  ///* query item level */
  //cstable::BitPackedIntColumnWriter jqi_position_col(2, 2, 64);
  //cstable::BooleanColumnWriter jqi_clicked_col(2, 2);

  //uint64_t r = 0;
  //uint64_t n = 0;

  auto add_session = [&schema, &analyzer] (const cm::JoinedSession& sess) {
    msg::MessageBuilder msg;

    for (int j = 0; j < sess.queries.size(); ++j) {
      auto q = sess.queries[j];
      auto qprefix = StringUtil::format("queries[$0].", j);

      /* queries.time */
      msg.setUInt32(qprefix + "time", q.time.unixMicros() / kMicrosPerSecond);

      /* queries.language */
      auto lang = cm::extractLanguage(q.attrs);
      msg.setUInt32(qprefix + "language", (uint16_t) lang);

      /* queries.query_string */
      auto qstr = cm::extractQueryString(q.attrs);
      if (!qstr.isEmpty()) {
        auto qstr_norm = analyzer.normalize(lang, qstr.get());
        msg.setString(qprefix + "query_string", qstr.get());
        msg.setString(qprefix + "query_string_normalized", qstr_norm);
      }

      /* queries.num_item_clicks, queries.num_items */
      size_t nitems = 0;
      size_t nclicks = 0;
      size_t nads = 0;
      size_t nadclicks = 0;
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

      msg.setUInt32(qprefix + "num_items", nitems);
      msg.setUInt32(qprefix + "num_items_clicked", nclicks);
      msg.setUInt32(qprefix + "num_ad_impressions", nads);
      msg.setUInt32(qprefix + "num_ad_clicks", nadclicks);

      /* queries.page */
      auto pg_str = cm::extractAttr(q.attrs, "pg");
      if (!pg_str.isEmpty()) {
        msg.setUInt32(qprefix + "page", std::stoul(pg_str.get()));
      }

      /* queries.ab_test_group */
      auto abgrp = cm::extractABTestGroup(q.attrs);
      if (!abgrp.isEmpty()) {
        msg.setUInt32(qprefix + "ab_test_group", abgrp.get());
      }

      /* queries.category1 */
      auto qcat1 = cm::extractAttr(q.attrs, "q_cat1");
      if (!qcat1.isEmpty()) {
        msg.setUInt32(qprefix + "category1", std::stoul(qcat1.get()));
      }

      /* queries.category2 */
      auto qcat2 = cm::extractAttr(q.attrs, "q_cat2");
      if (!qcat2.isEmpty()) {
        msg.setUInt32(qprefix + "category2", std::stoul(qcat2.get()));
      }

      /* queries.category3 */
      auto qcat3 = cm::extractAttr(q.attrs, "q_cat3");
      if (!qcat3.isEmpty()) {
        msg.setUInt32(qprefix + "category3", std::stoul(qcat3.get()));
      }

      /* queries.device_type */
      msg.setUInt32(
          qprefix + "device_type",
          (uint32_t) extractDeviceType(q.attrs));

      /* queries.page_type */
      msg.setUInt32(
          qprefix + "page_type",
          (uint32_t) extractPageType(q.attrs));


      //if (q.items.size() == 0) {
      //  jqi_position_col.addNull(r, 1);
      //  jqi_clicked_col.addNull(r, 1);
      //}

      //for (const auto& i : q.items) {
      //  jqi_position_col.addDatum(r, 2, i.position);
      //  jqi_clicked_col.addDatum(r, 2, i.clicked);
      //  r = 2;
      //}
    }

    Buffer msg_buf;
    msg.encode(schema, &msg_buf);

    fnord::iputs("msg: $0", StringUtil::hexPrint(msg_buf.data(), msg_buf.size()));
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

  //{
  //  cstable::CSTableWriter writer(flags.getString("output_file") + "~", n);
  //  writer.addColumn("queries.time", &jq_time_col);
  //  writer.addColumn("queries.page", &jq_page_col);
  //  writer.addColumn("queries.language", &jq_lang_col);
  //  writer.addColumn("queries.query_string", &jq_qstr_col);
  //  writer.addColumn("queries.query_string_normalized", &jq_qstrnorm_col);
  //  writer.addColumn("queries.num_items", &jq_numitems_col);
  //  writer.addColumn("queries.num_items_clicked", &jq_numitemclicks_col);
  //  writer.addColumn("queries.num_ad_impressions", &jq_numadimprs_col);
  //  writer.addColumn("queries.num_ad_clicks", &jq_numadclicks_col);
  //  writer.addColumn("queries.ab_test_group", &jq_abtestgroup_col);
  //  writer.addColumn("queries.device_type", &jq_devicetype_col);
  //  writer.addColumn("queries.page_type", &jq_pagetype_col);
  //  writer.addColumn("queries.category1", &jq_cat1_col);
  //  writer.addColumn("queries.category2", &jq_cat2_col);
  //  writer.addColumn("queries.category3", &jq_cat3_col);
  //  writer.addColumn("queries.items.position", &jqi_position_col);
  //  writer.addColumn("queries.items.clicked", &jqi_clicked_col);
  //  writer.commit();
  //}

  //FileUtil::mv(
  //    flags.getString("output_file") + "~",
  //    flags.getString("output_file"));

  //{
  //  cstable::CSTableReader reader(flags.getString("output_file"));
  //  auto t0 = WallClock::unixMicros();

  //  cm::AnalyticsTableScan aq;
  //  auto lcol = aq.fetchColumn("queries.language");
  //  auto ccol = aq.fetchColumn("queries.num_ad_clicks");
  //  auto qcol = aq.fetchColumn("queries.query_string_normalized");

  //  aq.onQuery([&] () {
  //    auto l = languageToString((Language) lcol->getUInt32());
  //    auto c = ccol->getUInt32();
  //    auto q = qcol->getString();
  //    fnord::iputs("lang: $0 -> $1 -- $2", l, c, q);
  //  });

  //  aq.scanTable(&reader);
  //  //cm::CTRByGroupResult<uint16_t> res;
  //  //cm::CTRByPositionQuery q(&aq, &res);
  //  //auto t1 = WallClock::unixMicros();
  //  //fnord::iputs("scanned $0 rows in $1 ms", res.rows_scanned, (t1 - t0) / 1000.0f);
  //  //for (const auto& p : res.counters) {
  //  //  fnord::iputs(
  //  //     "pos: $0, views: $1, clicks: $2, ctr: $3", 
  //  //      p.first, p.second.num_views,
  //  //      p.second.num_clicks,
  //  //      p.second.num_clicks / (double) p.second.num_views);
  //  //}
  //}

  return 0;
}

