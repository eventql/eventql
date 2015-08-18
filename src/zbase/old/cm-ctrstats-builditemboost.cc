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
#include "stx/mdb/MDB.h"
#include "stx/mdb/MDBUtil.h"
#include <fnord-fts/fts.h>
#include <fnord-fts/fts_common.h>
#include "sstable/sstablereader.h"
#include "sstable/SSTableEditor.h"
#include "sstable/SSTableColumnSchema.h"
#include "sstable/SSTableColumnReader.h"
#include "sstable/SSTableColumnWriter.h"
#include "common.h"
#include "CustomerNamespace.h"

#include "IndexChangeRequest.h"
#
#include "zbase/CTRCounter.h"

using namespace stx;
using namespace zbase;

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
    const zbase::JoinedQuery& query,
    ItemEligibility eligibility,
    //FeatureIndex* feature_index,
    stx::fts::Analyzer* analyzer,
    Language lang,
    CounterMap* counters,
    GlobalCounter* global_counter) {
  if (!isQueryEligible(eligibility, query)) {
    return;
  }

  /* query string terms */
  Set<String> qstr_terms;
  auto qstr_opt = zbase::extractAttr(query.attrs, "qstr~de"); // FIXPAUL
  if (!qstr_opt.isEmpty()) {
    analyzer->extractTerms(lang, qstr_opt.get(), &qstr_terms);
  }

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
    CounterMap& counters,
    const GlobalCounter& global_counter,
    uint64_t start_time,
    uint64_t end_time,
    bool rollup,
    int min_views,
    int min_clicks,
    int write_index_requests) {
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
  double ctr_stddev;
  double ctr_base_stddev;

  if (rollup) {
    for (auto iter = counters.begin(); iter != counters.end(); ) {
      if (iter->second.views >= min_views && iter->second.clicks >= min_clicks) {
        ++iter;
      } else {
        iter = counters.erase(iter);
      }
    }

    double ctr_mean_num = 0.0;
    double ctr_stddev_num = 0.0;
    double ctr_base_mean_num = 0.0;
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

      ctr_stddev_num += pow(ctr - ctr_mean, 2.0);
      ctr_base_stddev_num += pow(ctr_base - ctr_base_mean, 2.0);
    }

    ctr_stddev = sqrt(ctr_stddev_num / n);
    ctr_base_stddev = sqrt(ctr_base_stddev_num / n);
  }

  ///* open output sstable */
  auto sstable_writer = sstable::SSTableEditor::create(
      filename,
      sstable::IndexProvider{},
      outhdr_json.data(),
      outhdr_json.length());

  for (const auto& c : counters) {
    auto ctr = c.second.clicks / (double) c.second.views;
    auto ctr_base =
        (c.second.clicks_base / (double) m) / (double) c.second.views;
    auto ctr_std = (ctr - ctr_mean) / ctr_stddev;
    auto ctr_base_std = (ctr_base - ctr_base_mean) / ctr_base_stddev;

    String terms_str;
    for (const auto t : c.second.term_counts) {
      terms_str += StringUtil::format(
          rollup ? "$0," : "$0:$1,",
          intern_map.getString(t.first),
          t.second);
    }

    if (write_index_requests) {
      zbase::IndexChangeRequest idx_req;
      idx_req.item.item_id = StringUtil::toString(c.first);
      idx_req.item.set_id = "p";
      idx_req.attrs["cm_views"] = StringUtil::toString(c.second.views);
      idx_req.attrs["cm_clicks"] = StringUtil::toString(c.second.clicks);
      idx_req.attrs["cm_ctr"] = StringUtil::toString(ctr);
      idx_req.attrs["cm_ctr_norm"] = StringUtil::toString(ctr_base);
      idx_req.attrs["cm_ctr_std"] = StringUtil::toString(ctr_std);
      idx_req.attrs["cm_ctr_norm_std"] = StringUtil::toString(ctr_base_std);
      idx_req.attrs["cm_clicked_terms"] = terms_str;
      sstable_writer->appendRow("", json::toJSONString(idx_req));
    } else {
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

  //flags.defineFlag(
  //    "featuredb_path",
  //    cli::FlagParser::T_STRING,
  //    true,
  //    NULL,
  //    NULL,
  //    "feature db path",
  //    "<path>");

  flags.defineFlag(
      "merge",
      stx::cli::FlagParser::T_SWITCH,
      false,
      NULL,
      NULL,
      "merge pre-processed tables",
      "");

  flags.defineFlag(
      "rollup",
      stx::cli::FlagParser::T_SWITCH,
      false,
      NULL,
      NULL,
      "write aggregated output columns",
      "");

  flags.defineFlag(
      "write_index_requests",
      stx::cli::FlagParser::T_SWITCH,
      false,
      NULL,
      NULL,
      "write index requests instead of raw table rows",
      "");

  flags.defineFlag(
      "min_views",
      stx::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "100",
      "minimum nuber of item views",
      "<num>");

  flags.defineFlag(
      "min_clicks",
      stx::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "10",
      "minimum nuber of item views",
      "<num>");

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
  GlobalCounter global_counter;
  auto start_time = std::numeric_limits<uint64_t>::max();
  auto end_time = std::numeric_limits<uint64_t>::min();

  auto lang = languageFromString(flags.getString("lang"));
  stx::fts::Analyzer analyzer(flags.getString("conf"));

  /* set up posi norm */
  posi_norm.emplace(1 , 1.0 / 0.006592);
  posi_norm.emplace(2 , 1.0 / 0.006428);
  posi_norm.emplace(3 , 1.0 / 0.006280);
  posi_norm.emplace(4 , 1.0 / 0.004807);
  posi_norm.emplace(5 , 1.0 / 0.014855);
  posi_norm.emplace(6 , 1.0 / 0.011013);
  posi_norm.emplace(7 , 1.0 / 0.009650);
  posi_norm.emplace(8 , 1.0 / 0.009113);
  posi_norm.emplace(9 , 1.0 / 0.007431);
  posi_norm.emplace(10, 1.0 / 0.007575);
  posi_norm.emplace(11, 1.0 / 0.006746);
  posi_norm.emplace(12, 1.0 / 0.006797);
  posi_norm.emplace(13, 1.0 / 0.006326);
  posi_norm.emplace(14, 1.0 / 0.006120);
  posi_norm.emplace(15, 1.0 / 0.005872);
  posi_norm.emplace(16, 1.0 / 0.005912);
  posi_norm.emplace(17, 1.0 / 0.005468);
  posi_norm.emplace(18, 1.0 / 0.005541);
  posi_norm.emplace(19, 1.0 / 0.005432);
  posi_norm.emplace(20, 1.0 / 0.005267);
  posi_norm.emplace(21, 1.0 / 0.005092);
  posi_norm.emplace(22, 1.0 / 0.005211);
  posi_norm.emplace(23, 1.0 / 0.004906);
  posi_norm.emplace(24, 1.0 / 0.004943);
  posi_norm.emplace(25, 1.0 / 0.004797);
  posi_norm.emplace(26, 1.0 / 0.004828);
  posi_norm.emplace(27, 1.0 / 0.004719);
  posi_norm.emplace(28, 1.0 / 0.004761);
  posi_norm.emplace(29, 1.0 / 0.004475);
  posi_norm.emplace(30, 1.0 / 0.004629);
  posi_norm.emplace(31, 1.0 / 0.004537);
  posi_norm.emplace(32, 1.0 / 0.004463);
  posi_norm.emplace(33, 1.0 / 0.004437);
  posi_norm.emplace(34, 1.0 / 0.004544);
  posi_norm.emplace(35, 1.0 / 0.004429);
  posi_norm.emplace(36, 1.0 / 0.004562);
  posi_norm.emplace(37, 1.0 / 0.004946);
  posi_norm.emplace(38, 1.0 / 0.005049);
  posi_norm.emplace(39, 1.0 / 0.005191);
  posi_norm.emplace(40, 1.0 / 0.005817);
  posi_norm.emplace(41, 1.0 / 0.006957);
  posi_norm.emplace(42, 1.0 / 0.007209);
  posi_norm.emplace(43, 1.0 / 0.007507);
  posi_norm.emplace(44, 1.0 / 0.007008);

  /* set up feature schema */
  //zbase::FeatureSchema feature_schema;
  //feature_schema.registerFeature("shop_id", 1, 1);
  //feature_schema.registerFeature("category1", 2, 1);
  //feature_schema.registerFeature("category2", 3, 1);
  //feature_schema.registerFeature("category3", 4, 1);
  //feature_schema.registerFeature("title~de", 5, 2);

  ///* open featuredb db */
  //auto featuredb_path = flags.getString("featuredb_path");
  //auto featuredb = mdb::MDB::open(featuredb_path, true);
  //zbase::FeatureIndex feature_index(featuredb, &feature_schema);

  /* read input tables */
  auto sstables = flags.getArgv();
  auto tbl_cnt = sstables.size();
  auto merge = flags.isSet("merge");
  for (int tbl_idx = 0; tbl_idx < sstables.size(); ++tbl_idx) {
    const auto& sstable = sstables[tbl_idx];
    stx::logInfo("cm.ctrstats", "Importing sstable: $0", sstable);

    /* read sstable header */
    sstable::SSTableReader reader(File::openFile(sstable, File::O_READ));

    if (reader.bodySize() == 0) {
      stx::logCritical("cm.ctrstats", "unfinished sstable: $0", sstable);
      exit(1);
    }

    sstable::SSTableColumnSchema schema;
    if (merge) {
      schema.loadIndex(&reader);
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

      stx::logInfo(
          "cm.ctrstats",
          "[$0%] Reading sstables... rows=$1",
          (size_t) (p * 100),
          row_idx);
    });

    /* read sstable rows */
    for (; cursor->valid(); ++row_idx) {
      status_line.runMaybe();

      if (merge) {
        auto key = std::stoul(cursor->getKeyString());
        auto val = cursor->getDataBuffer();
        sstable::SSTableColumnReader cols(&schema, val);

        auto& c = counters[key];
        c.views += cols.getUInt64Column(schema.columnID("views"));
        c.clicks += cols.getUInt64Column(schema.columnID("clicks"));
        c.clicks_base += cols.getFloatColumn(schema.columnID("clicks_base"));

        auto terms_str = cols.getStringColumn(schema.columnID("terms"));
        for (const auto& term : StringUtil::split(terms_str, ",")) {
          auto len = term.find(":");
          if (len == std::string::npos) {
            continue;
          }

          auto t_str = term.substr(0, len);
          auto t_cnt = std::stoul(term.substr(len + 1));
          c.term_counts[intern_map.internString(t_str)] += t_cnt;
        }
      } else {
        auto val = cursor->getDataBuffer();
        Option<zbase::JoinedQuery> q;

        try {
          q = Some(json::fromJSON<zbase::JoinedQuery>(val));
        } catch (const Exception& e) {
          stx::logWarning(
              "cm.ctrstats",
              e,
              "invalid json: $0",
              val.toString());
        }

        try {
          if (!q.isEmpty()) {
            indexJoinedQuery(
                q.get(),
                zbase::ItemEligibility::DAWANDA_ALL_NOBOTS,
                //&feature_index,
                &analyzer,
                lang,
                &counters,
                &global_counter);
          }
        } catch (const Exception& e) {
          stx::logError("cm.ctrstats", e, "error while indexing query");
        }
      }

      if (!cursor->next()) {
        break;
      }
    }

    status_line.runForce();
  }

  /* write output table */
  auto min_views = flags.getInt("min_views");
  auto min_clicks = flags.getInt("min_clicks");
  auto outfile = flags.getString("output_file");
  stx::logInfo("cm.ctrstats", "Writing results to: $0", outfile);

  if (FileUtil::exists(outfile + "~")) {
    stx::logInfo(
        "cm.ctrstats",
        "Deleting orphaned temp file: $0",
        outfile + "~");

    FileUtil::rm(outfile + "~");
  }

  writeOutputTable(
      outfile + "~",
      counters,
      global_counter,
      start_time,
      end_time,
      flags.isSet("rollup"),
      min_views,
      min_clicks,
      flags.isSet("write_index_requests"));

  FileUtil::mv(outfile + "~", outfile);

  return 0;
}

