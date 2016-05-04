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
#include "stx/io/fileutil.h"
#include "stx/application.h"
#include "stx/logging.h"
#include "stx/Language.h"
#include "stx/cli/flagparser.h"
#include "stx/util/SimpleRateLimit.h"
#include "stx/InternMap.h"
#include "stx/json/json.h"
#include "eventql/util/mdb/MDB.h"
#include "eventql/util/mdb/MDBUtil.h"
#include "sstable/sstablereader.h"
#include "sstable/SSTableEditor.h"
#include "sstable/SSTableColumnSchema.h"
#include "sstable/SSTableColumnReader.h"
#include "sstable/SSTableColumnWriter.h"
#include "common.h"
#include "CustomerNamespace.h"

#
#include "eventql/CTRCounter.h"
#include <eventql/util/fts.h>
#include <eventql/util/fts_common.h>

using namespace stx;
using namespace zbase;

typedef Tuple<String, uint64_t, uint64_t> OutputRow;
typedef HashMap<String, zbase::CTRCounterData> CounterMap;

InternMap intern_map;

void indexJoinedQuery(
    const zbase::JoinedQuery& query,
    const String& query_feature_name,
    mdb::MDB* featuredb,
    FeatureIndex* feature_index,
    FeatureID item_feature_id,
    ItemEligibility eligibility,
    stx::fts::Analyzer* analyzer,
    Language lang,
    CounterMap* counters) {
  if (!isQueryEligible(eligibility, query)) {
    return;
  }

  auto fstr_opt = zbase::extractAttr(query.attrs, query_feature_name);
  if (fstr_opt.isEmpty()) {
    return;
  }

  auto fstr = URI::urlDecode(fstr_opt.get());
  auto& global_counter = (*counters)[""];

  for (const auto& item : query.items) {
    if (!isItemEligible(eligibility, query, item)) {
      continue;
    }

    Option<String> ifstr_opt;

    try {
      ifstr_opt = feature_index->getFeature(
          item.item.docID(),
          item_feature_id);
    } catch (const Exception& e) {
      stx::logError("cm.ctrstatsbuild", e, "error");
    }

   if (ifstr_opt.isEmpty()) {
      continue;
    }

    Set<String> tokens;
    analyzer->extractTerms(lang, ifstr_opt.get(), &tokens);

    for (const auto& token : tokens) {
      Buffer counter_key;
      Buffer group_counter_key;
      void* tmp = intern_map.internString(fstr);
      counter_key.append(&tmp, sizeof(tmp));
      group_counter_key.append(&tmp, sizeof(tmp));
      tmp = intern_map.internString(token);
      counter_key.append(&tmp, sizeof(tmp));

      auto& counter = (*counters)[counter_key.toString()];
      auto& group_counter = (*counters)[group_counter_key.toString()];

      counter.num_views++;
      group_counter.num_views++;
      global_counter.num_views++;

      if (item.clicked) {
        counter.num_clicks++;
        group_counter.num_clicks++;
        global_counter.num_clicks++;
      }
    }
  }
}

/* write output table */
void writeOutputTable(
    const String& filename,
    const CounterMap& counters,
    uint64_t start_time,
    uint64_t end_time) {
  /* prepare output sstable schema */
  sstable::SSTableColumnSchema sstable_schema;
  sstable_schema.addColumn("num_views", 1, sstable::SSTableColumnType::UINT64);
  sstable_schema.addColumn("num_clicks", 2, sstable::SSTableColumnType::UINT64);
  sstable_schema.addColumn("num_clicked", 3, sstable::SSTableColumnType::UINT64);

  HashMap<String, String> out_hdr;
  out_hdr["start_time"] = StringUtil::toString(start_time);
  out_hdr["end_time"] = StringUtil::toString(end_time);
  auto outhdr_json = json::toJSONString(out_hdr);

  /* open output sstable */
  stx::logInfo("cm.ctrstats", "Writing results to: $0", filename);
  auto sstable_writer = sstable::SSTableEditor::create(
      filename,
      sstable::IndexProvider{},
      outhdr_json.data(),
      outhdr_json.length());


  for (const auto& p : counters) {
    sstable::SSTableColumnWriter cols(&sstable_schema);
    cols.addUInt64Column(1, p.second.num_views);
    cols.addUInt64Column(2, p.second.num_clicks);
    cols.addUInt64Column(3, p.second.num_clicked);

    String key_str;
    switch (p.first.length()) {

      case 0: {
        key_str = "__GLOBAL";
        break;
      }

      case (sizeof(void*)): {
        key_str = intern_map.getString(((void**) p.first.c_str())[0]);
        break;
      }

      case (sizeof(void*) * 2): {
        key_str = intern_map.getString(((void**) p.first.c_str())[0]);
        key_str += "~";
        key_str += intern_map.getString(((void**) p.first.c_str())[1]);
        break;
      }

      default:
        RAISE(kRuntimeError, "invalid counter key");

    }

    sstable_writer->appendRow(key_str, cols);
  }

  sstable_schema.writeIndex(sstable_writer.get());
  sstable_writer->finalize();
}

int main(int argc, const char** argv) {
  stx::Application::init();
  stx::Application::logToStderr();

  stx::cli::FlagParser flags;

  flags.defineFlag(
      "lang",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "language",
      "<lang>");

  flags.defineFlag(
      "conf",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      "./conf",
      "conf directory",
      "<path>");

  flags.defineFlag(
      "output_file",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "output file path",
      "<path>");

  flags.defineFlag(
      "query_feature",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "query feature",
      "<feature>");

  flags.defineFlag(
      "item_feature",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "item feature",
      "<feature>");

  flags.defineFlag(
      "featuredb_path",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "feature db path",
      "<path>");

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

  CounterMap counters;
  auto query_feature = flags.getString("query_feature");
  auto start_time = std::numeric_limits<uint64_t>::max();
  auto end_time = std::numeric_limits<uint64_t>::min();

  auto lang = languageFromString(flags.getString("lang"));
  stx::fts::Analyzer analyzer(flags.getString("conf"));

  /* set up feature schema */
  zbase::FeatureSchema feature_schema;
  feature_schema.registerFeature("shop_id", 1, 1);
  feature_schema.registerFeature("category1", 2, 1);
  feature_schema.registerFeature("category2", 3, 1);
  feature_schema.registerFeature("category3", 4, 1);
  feature_schema.registerFeature("title~de", 5, 2);

  /* open featuredb db */
  auto featuredb_path = flags.getString("featuredb_path");
  auto featuredb = mdb::MDB::open(featuredb_path, true);
  zbase::FeatureIndex feature_index(featuredb, &feature_schema);

  /* read input tables */
  auto sstables = flags.getArgv();
  for (int tbl_idx = 0; tbl_idx < sstables.size(); ++tbl_idx) {
    const auto& sstable = sstables[tbl_idx];
    stx::logInfo("cm.ctrstats", "Importing sstable: $0", sstable);

    /* read sstable header */
    sstable::SSTableReader reader(File::openFile(sstable, File::O_READ));

    if (reader.bodySize() == 0) {
      stx::logCritical("cm.ctrstats", "unfinished sstable: $0", sstable);
      exit(1);
    }

    /* read report header */
    auto hdr = json::parseJSON(reader.readHeader());

    auto tbl_start_time = json::JSONUtil::objectGetUInt64(
        hdr.begin(),
        hdr.end(),
        "start_time").get();

    auto tbl_end_time = json::JSONUtil::objectGetUInt64(
        hdr.begin(),
        hdr.end(),
        "end_time").get();

    if (tbl_start_time < start_time) {
      start_time = tbl_start_time;
    }

    if (tbl_end_time > end_time) {
      end_time = tbl_end_time;
    }

    /* get sstable cursor */
    auto cursor = reader.getCursor();
    auto body_size = reader.bodySize();
    int row_idx = 0;

    /* status line */
    util::SimpleRateLimitedFn status_line(kMicrosPerSecond, [&] () {
      stx::logInfo(
          "cm.ctrstats",
          "[$1/$2] [$0%] Reading sstable... rows=$3",
          (size_t) ((cursor->position() / (double) body_size) * 100),
          tbl_idx + 1, sstables.size(), row_idx);
    });

    /* read sstable rows */
    for (; cursor->valid(); ++row_idx) {
      status_line.runMaybe();

      auto val = cursor->getDataBuffer();
      Option<zbase::JoinedQuery> q;

      try {
        q = Some(json::fromJSON<zbase::JoinedQuery>(val));
      } catch (const Exception& e) {
        //stx::logWarning("cm.ctrstats", e, "invalid json: $0", val.toString());
      }

      if (!q.isEmpty()) {
        indexJoinedQuery(
            q.get(),
            query_feature,
            featuredb.get(),
            &feature_index,
            feature_schema.featureID(flags.getString("item_feature")).get(),
            zbase::ItemEligibility::DAWANDA_ALL_NOBOTS,
            &analyzer,
            lang,
            &counters);
      }

      if (!cursor->next()) {
        break;
      }
    }

    status_line.runForce();
  }

  /* write output table */
  writeOutputTable(
      flags.getString("output_file"),
      counters,
      start_time,
      end_time);

  return 0;
}

