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
#include "fnord-base/Language.h"
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
#include "common.h"
#include "CustomerNamespace.h"
#include "FeatureSchema.h"
#include "JoinedQuery.h"
#include "CTRCounter.h"
#include "Analyzer.h"

using namespace fnord;
using namespace cm;

struct ItemStats {
  ItemStats() : views(0), clicks(0), clicks_base(0) {}
  uint32_t views;
  uint32_t clicks;
  uint32_t clicks_base;
  HashMap<void*, uint32_t> term_counts;
};

struct GlobalCounter {
  GlobalCounter() : views(0), clicks(0) {}
  uint32_t views;
  uint32_t clicks;
};

typedef HashMap<uint64_t, ItemStats> CounterMap;

InternMap intern_map;

HashMap<uint32_t, double> posi_norm;

void indexJoinedQuery(
    const cm::JoinedQuery& query,
    ItemEligibility eligibility,
    FeatureIndex* feature_index,
    Analyzer* analyzer,
    Language lang,
    CounterMap* counters,
    GlobalCounter* global_counter) {
  if (!isQueryEligible(eligibility, query)) {
    return;
  }

  /* query string terms */
  auto qstr_opt = cm::extractAttr(query.attrs, "qstr~de"); // FIXPAUL
  if (qstr_opt.isEmpty()) {
    return;
  }

  Set<String> qstr_terms;
  analyzer->extractTerms(lang, qstr_opt.get(), &qstr_terms);

  for (const auto& item : query.items) {
    if (!isItemEligible(eligibility, query, item)) {
      continue;
    }

    auto& stats = (*counters)[std::stoul(item.item.item_id)];
    ++stats.views;
    ++global_counter->views;

    if (item.clicked) {
      stats.clicks += 1;
      stats.clicks_base += posi_norm[item.position];
      global_counter->clicks += 1;

      for (const auto& term : qstr_terms) {
        ++stats.term_counts[intern_map.internString(term)];
      }
    }
  }
}

/* write output table */
void writeOutputTable(
    const String& filename,
    const CounterMap& counters,
    const GlobalCounter& global_counter,
    uint64_t start_time,
    uint64_t end_time,
    bool rollup) {
  /* prepare output sstable schema */
  sstable::SSTableColumnSchema sstable_schema;
  sstable_schema.addColumn("views", 1, sstable::SSTableColumnType::UINT64);
  sstable_schema.addColumn("clicks", 2, sstable::SSTableColumnType::UINT64);
  sstable_schema.addColumn("clicks_base", 3, sstable::SSTableColumnType::FLOAT);
  sstable_schema.addColumn("terms", 4, sstable::SSTableColumnType::STRING);

  if (rollup) {
    sstable_schema.addColumn("ctr", 5, sstable::SSTableColumnType::FLOAT);
    sstable_schema.addColumn("ctr_base", 6, sstable::SSTableColumnType::FLOAT);
    sstable_schema.addColumn("ctr_std", 7, sstable::SSTableColumnType::FLOAT);
    sstable_schema.addColumn("ctr_base_std", 8, sstable::SSTableColumnType::FLOAT);
  }

  HashMap<String, String> out_hdr;
  out_hdr["start_time"] = StringUtil::toString(start_time);
  out_hdr["end_time"] = StringUtil::toString(end_time);
  auto outhdr_json = json::toJSONString(out_hdr);

  double n = counters.size();
  double m = posi_norm.size();
  double ctr_mean;
  double ctr_base_mean;

  if (rollup) {
    double ctr_mean_num = 0.0;
    double ctr_stddev = 0.0;
    double ctr_stddev_num = 0.0;
    double ctr_base_mean_num = 0.0;
    double ctr_base_stddev = 0.0;
    double ctr_base_stddev_num = 0.0;

    for (const auto& c : counters) {
      auto ctr = c.second.clicks / (double) c.second.views;
      auto ctr_base =
          (c.second.clicks_base / (double) m) / (double) c.second.views;

      ctr_mean_num += ctr;
      ctr_base_mean_num += ctr_base;
    }

    ctr_mean = ctr_mean_num / n;
    ctr_base_mean = ctr_base_mean_num / n;

    for (const auto& c : counters) {
      auto ctr = c.second.clicks / (double) c.second.views;
      auto ctr_base =
          (c.second.clicks_base / (double) m) / (double) c.second.views;

    }
  }

  ///* open output sstable */
  fnord::logInfo("cm.ctrstats", "Writing results to: $0", filename);
  auto sstable_writer = sstable::SSTableWriter::create(
      filename,
      sstable::IndexProvider{},
      outhdr_json.data(),
      outhdr_json.length());

  for (const auto& c : counters) {
    auto ctr = c.second.clicks / (double) c.second.views;
    auto ctr_base =
        (c.second.clicks_base / (double) m) / (double) c.second.views;
    auto ctr_std = ctr - ctr_mean;
    auto ctr_base_std = ctr - ctr_base_mean;

    String terms_str;
    for (const auto t : c.second.term_counts) {
      terms_str += StringUtil::format(
          rollup ? "$0," : "$0:$1,",
          intern_map.getString(t.first),
          t.second);
    }

    sstable::SSTableColumnWriter cols(&sstable_schema);
    cols.addUInt64Column(1, c.second.views);
    cols.addUInt64Column(2, c.second.clicks);
    cols.addFloatColumn(3, c.second.clicks_base);

    if (rollup) {
      cols.addFloatColumn(5, ctr);
      cols.addFloatColumn(6, ctr_base);
      cols.addFloatColumn(7, ctr_std);
      cols.addFloatColumn(8, ctr_base_std);
    }

    cols.addStringColumn(4, terms_str);

    sstable_writer->appendRow(StringUtil::toString(c.first), cols);
  }

  sstable_schema.writeIndex(sstable_writer.get());
  sstable_writer->finalize();
}

int main(int argc, const char** argv) {
  fnord::Application::init();
  fnord::Application::logToStderr();

  fnord::cli::FlagParser flags;

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
      "featuredb_path",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "feature db path",
      "<path>");

  flags.defineFlag(
      "merge",
      fnord::cli::FlagParser::T_SWITCH,
      false,
      NULL,
      NULL,
      "merge pre-processed tables",
      "");

  flags.defineFlag(
      "rollup",
      fnord::cli::FlagParser::T_SWITCH,
      false,
      NULL,
      NULL,
      "write aggregated output columns",
      "");

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

  CounterMap counters;
  GlobalCounter global_counter;
  auto start_time = std::numeric_limits<uint64_t>::max();
  auto end_time = std::numeric_limits<uint64_t>::min();

  auto lang = languageFromString(flags.getString("lang"));
  cm::Analyzer analyzer(flags.getString("conf"));

  /* set up posi norm */
  posi_norm.emplace(1,  1.0 / 0.006728);
  posi_norm.emplace(2,  1.0 / 0.006491);
  posi_norm.emplace(3,  1.0 / 0.006345);
  posi_norm.emplace(4,  1.0 / 0.004955);
  posi_norm.emplace(5,  1.0 / 0.015407);
  posi_norm.emplace(6,  1.0 / 0.010970);
  posi_norm.emplace(7,  1.0 / 0.009629);
  posi_norm.emplace(8,  1.0 / 0.009070);
  posi_norm.emplace(9,  1.0 / 0.007370);
  posi_norm.emplace(10, 1.0 / 0.007518);
  posi_norm.emplace(11, 1.0 / 0.006699);
  posi_norm.emplace(12, 1.0 / 0.006751);
  posi_norm.emplace(13, 1.0 / 0.006243);
  posi_norm.emplace(14, 1.0 / 0.006058);
  posi_norm.emplace(15, 1.0 / 0.005885);
  posi_norm.emplace(16, 1.0 / 0.005909);
  posi_norm.emplace(17, 1.0 / 0.005453);
  posi_norm.emplace(18, 1.0 / 0.005475);
  posi_norm.emplace(19, 1.0 / 0.005424);
  posi_norm.emplace(20, 1.0 / 0.005240);
  posi_norm.emplace(21, 1.0 / 0.005058);
  posi_norm.emplace(22, 1.0 / 0.005181);
  posi_norm.emplace(23, 1.0 / 0.004913);
  posi_norm.emplace(24, 1.0 / 0.004935);
  posi_norm.emplace(25, 1.0 / 0.004856);
  posi_norm.emplace(26, 1.0 / 0.004857);
  posi_norm.emplace(27, 1.0 / 0.004649);
  posi_norm.emplace(28, 1.0 / 0.004716);
  posi_norm.emplace(29, 1.0 / 0.004446);
  posi_norm.emplace(30, 1.0 / 0.004694);
  posi_norm.emplace(31, 1.0 / 0.004542);
  posi_norm.emplace(32, 1.0 / 0.004394);
  posi_norm.emplace(33, 1.0 / 0.004469);
  posi_norm.emplace(34, 1.0 / 0.004522);
  posi_norm.emplace(35, 1.0 / 0.004416);
  posi_norm.emplace(36, 1.0 / 0.004576);
  posi_norm.emplace(37, 1.0 / 0.004984);
  posi_norm.emplace(38, 1.0 / 0.005158);
  posi_norm.emplace(39, 1.0 / 0.005268);
  posi_norm.emplace(40, 1.0 / 0.005833);

  /* set up feature schema */
  cm::FeatureSchema feature_schema;
  feature_schema.registerFeature("shop_id", 1, 1);
  feature_schema.registerFeature("category1", 2, 1);
  feature_schema.registerFeature("category2", 3, 1);
  feature_schema.registerFeature("category3", 4, 1);
  feature_schema.registerFeature("title~de", 5, 2);

  /* open featuredb db */
  auto featuredb_path = flags.getString("featuredb_path");
  auto featuredb = mdb::MDB::open(featuredb_path, true);
  cm::FeatureIndex feature_index(featuredb, &feature_schema);

  /* read input tables */
  auto sstables = flags.getArgv();
  auto tbl_cnt = sstables.size();
  for (int tbl_idx = 0; tbl_idx < sstables.size(); ++tbl_idx) {
    const auto& sstable = sstables[tbl_idx];
    fnord::logInfo("cm.ctrstats", "Importing sstable: $0", sstable);

    /* read sstable header */
    sstable::SSTableReader reader(File::openFile(sstable, File::O_READ));

    if (reader.bodySize() == 0) {
      fnord::logCritical("cm.ctrstats", "unfinished sstable: $0", sstable);
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
      auto p = (tbl_idx / (double) tbl_cnt) +
          ((cursor->position() / (double) body_size)) / (double) tbl_cnt;

      fnord::logInfo(
          "cm.ctrstats",
          "[$0%] Reading sstables... rows=$1",
          (size_t) (p * 100),
          row_idx);
    });

    /* read sstable rows */
    for (; cursor->valid(); ++row_idx) {
      status_line.runMaybe();

      auto val = cursor->getDataBuffer();
      Option<cm::JoinedQuery> q;

      try {
        q = Some(json::fromJSON<cm::JoinedQuery>(val));
      } catch (const Exception& e) {
        fnord::logWarning("cm.ctrstats", e, "invalid json: $0", val.toString());
      }

      if (!q.isEmpty()) {
        indexJoinedQuery(
            q.get(),
            cm::ItemEligibility::DAWANDA_ALL_NOBOTS,
            &feature_index,
            &analyzer,
            lang,
            &counters,
            &global_counter);
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
      global_counter,
      start_time,
      end_time,
      flags.isSet("rollup"));

  return 0;
}

