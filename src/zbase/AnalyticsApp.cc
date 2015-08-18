/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/wallclock.h"
#include "stx/util/Base64.h"
#include "zbase/AnalyticsApp.h"
#include "zbase/AnalyticsQueryMapper.h"
#include "zbase/AnalyticsQueryReducer.h"
#include "zbase/CSVExportRDD.h"
#include "zbase/JSONExportRDD.h"
#include "zbase/CSVSink.h"
#include "zbase/CTRCounterMergeReducer.h"
#include "zbase/ProtoSSTableMergeReducer.h"
#include "zbase/ProtoSSTableSink.h"
#include "zbase/AnalyticsSession.pb.h"
#include <tsdb/TSDBTableScanSpec.pb.h>
#include "tsdb/TimeWindowPartitioner.h"
#include <tsdb/CSTableIndex.h>
#include "tsdb/SQLEngine.h"
#include "zbase/SessionSchema.h"
#include "stx/protobuf/DynamicMessage.h"
#include "stx/protobuf/MessageEncoder.h"

#include "zbase/ecommerce/ECommerceKPIQuery.h"
#include "zbase/discovery/CTRByPositionQuery.h"
#include "zbase/discovery/CTRByPageQuery.h"
#include "zbase/discovery/DiscoveryDashboardQuery.h"
#include "zbase/discovery/ItemBoostMerge.h"
#include "zbase/discovery/ItemBoostMapper.h"
#include "zbase/discovery/ItemBoostExport.h"
#include "zbase/catalog/CatalogDashboardQuery.h"
#include "zbase/catalog/CatalogCategoryDashboardQuery.h"
#include "zbase/search/CTRByResultItemCategoryQuery.h"
#include "zbase/search/TopSearchTermsQuery.h"
#include "zbase/search/SearchDashboardQuery.h"
#include "zbase/search/TopSearchTermsReport.h"
#include "zbase/search/ECommerceSearchQueriesFeed.h"
#include "zbase/search/CTRBySearchTermCrossCategoryMapper.h"
#include "zbase/search/TopTermsByCategoryReport.h"
#include "zbase/search/TopCategoriesByTermMapper.h"
#include "zbase/search/RelatedTermsMapper.h"
#include "zbase/search/RelatedTermsReport.h"
#include "zbase/search/TopCategoriesByTermReport.h"
#include "zbase/reco_engine/RecoEngineStatsQuery.h"
#include "zbase/reco_engine/RecoEngineStatsBreakdownQuery.h"
#include "zbase/reco_engine/RecoEngineStatsReport.h"
#include "zbase/reco_engine/ECommerceRecoQueriesFeed.h"
#include "zbase/reco_engine/ECommercePreferenceSetsFeed.h"
#include "zbase/shop_stats/ShopProductsTable.h"
#include "zbase/shop_stats/ShopProductCTRStatsScan.h"
#include "zbase/shop_stats/ShopProductECommerceStatsScan.h"
#include "zbase/shop_stats/ShopCTRStatsScan.h"
#include "zbase/shop_stats/ShopECommerceStatsScan.h"
#include "zbase/shop_stats/ShopKPITable.h"
#include "zbase/shop_stats/ShopKPIDashboardQuery.h"
#include "zbase/shop_stats/ShopProductsDashboardQuery.h"

using namespace stx;

namespace cm {

AnalyticsApp::AnalyticsApp(
    tsdb::TSDBService* tsdb_node,
    tsdb::PartitionMap* partition_map,
    tsdb::ReplicationScheme* replication_scheme,
    tsdb::CSTableIndex* cstable_index,
    ConfigDirectory* cdb,
    AnalyticsAuth* auth,
    csql::Runtime* sql,
    const String& datadir) :
    dproc::DefaultApplication("cm.analytics"),
    tsdb_node_(tsdb_node),
    partition_map_(partition_map),
    cstable_index_(cstable_index),
    cdb_(cdb),
    datadir_(datadir),
    logfile_service_(
        cdb,
        auth,
        tsdb_node,
        partition_map,
        replication_scheme,
        sql) {
  cdb_->onCustomerConfigChange(
      std::bind(
          &AnalyticsApp::configureCustomer,
          this,
          std::placeholders::_1));

  cdb_->listCustomers([this] (const CustomerConfig& cfg) {
    configureCustomer(cfg);
  });

  cdb_->onTableDefinitionChange(
      std::bind(
          &AnalyticsApp::configureTable,
          this,
          std::placeholders::_1));

  cdb_->listTableDefinitions([this] (const TableDefinition& tbl) {
    configureTable(tbl);
  });

  queries_.registerQuery("discovery.CTRByPositionQuery", [] (
      const cm::AnalyticsQuery& query,
      const cm::AnalyticsQuery::SubQueryParams params,
      const Vector<RefPtr<cm::TrafficSegment>>& segments,
      cm::AnalyticsTableScan* scan) {
    return new cm::CTRByPositionQuery(scan, segments);
  });

  queries_.registerQuery("discovery.CTRByPageQuery", [] (
      const cm::AnalyticsQuery& query,
      const cm::AnalyticsQuery::SubQueryParams params,
      const Vector<RefPtr<cm::TrafficSegment>>& segments,
      cm::AnalyticsTableScan* scan) {
    return new cm::CTRByPageQuery(scan, segments);
  });

  registerProtoRDDFactory<tsdb::TSDBTableScanSpec>(
      "discovery.ItemBoostMapper",
      [this] (const tsdb::TSDBTableScanSpec& params)
          -> RefPtr<dproc::Task> {
        auto report = new ItemBoostMapper(
            new AnalyticsTableScanSource(params, tsdb_node_),
            new ProtoSSTableSink<ItemBoostRow>());

        report->setCacheKey(
          StringUtil::format(
              "cm.analytics.discovery.itemboostmapper~$0~$1~$2~$3",
              params.tsdb_namespace(),
              params.table_name(),
              params.partition_key(),
              params.version()));

        return report;
      });

  registerProtoRDDFactory<ReportParams>(
      "discovery.ItemBoostReducer",
      [this] (const ReportParams& params) -> RefPtr<dproc::Task> {
        auto map_chunks = AnalyticsTableScanPlanner::mapShards(
            params.customer(),
            params.from_unixmicros(),
            params.until_unixmicros(),
            "discovery.ItemBoostMapper",
            *msg::encode(params),
            tsdb_node_);

        return new ProtoSSTableMergeReducer<ItemBoostRow>(
            new ProtoSSTableSource<ItemBoostRow>(map_chunks),
            new ProtoSSTableSink<ItemBoostRow>(),
            std::bind(
                &ItemBoostMerge::merge,
                std::placeholders::_1,
                std::placeholders::_2));
      });

  registerTaskFactory(
      "discovery.ItemBoostCSV",
      [this] (const Buffer& params) -> RefPtr<dproc::Task> {
        List<dproc::TaskDependency> deps;
        deps.emplace_back(dproc::TaskDependency {
          .task_name = "discovery.ItemBoostReducer",
          .params = params
        });

        return new ItemBoostExport(
            new ProtoSSTableSource<ItemBoostRow>(deps),
            new CSVSink());
      });


  queries_.registerQuery("search.CTRByResultItemCategoryQuery", [] (
      const cm::AnalyticsQuery& query,
      const cm::AnalyticsQuery::SubQueryParams params,
      const Vector<RefPtr<cm::TrafficSegment>>& segments,
      cm::AnalyticsTableScan* scan) {
    return new cm::CTRByResultItemCategoryQuery(scan, segments);
  });


  queries_.registerQuery("search.SearchDashboardQuery", [] (
      const cm::AnalyticsQuery& query,
      const cm::AnalyticsQuery::SubQueryParams params,
      const Vector<RefPtr<cm::TrafficSegment>>& segments,
      cm::AnalyticsTableScan* scan) {
    return new cm::SearchDashboardQuery(
        scan,
        segments,
        query.start_time,
        query.end_time,
        params);
  });

  queries_.registerQuery("catalog.CatalogDashboardQuery", [] (
      const cm::AnalyticsQuery& query,
      const cm::AnalyticsQuery::SubQueryParams params,
      const Vector<RefPtr<cm::TrafficSegment>>& segments,
      cm::AnalyticsTableScan* scan) {
    return new cm::CatalogDashboardQuery(
        scan,
        segments,
        query.start_time,
        query.end_time,
        params);
  });

  queries_.registerQuery("catalog.CatalogCategoryE0DashboardQuery", [] (
      const cm::AnalyticsQuery& query,
      const cm::AnalyticsQuery::SubQueryParams params,
      const Vector<RefPtr<cm::TrafficSegment>>& segments,
      cm::AnalyticsTableScan* scan) {
    return new cm::CatalogCategoryDashboardQuery(
        scan,
        segments,
        query.start_time,
        query.end_time,
        "search_queries.category1",
        "search_queries.category1",
        params);
  });

  queries_.registerQuery("catalog.CatalogCategoryE1DashboardQuery", [] (
      const cm::AnalyticsQuery& query,
      const cm::AnalyticsQuery::SubQueryParams params,
      const Vector<RefPtr<cm::TrafficSegment>>& segments,
      cm::AnalyticsTableScan* scan) {
    return new cm::CatalogCategoryDashboardQuery(
        scan,
        segments,
        query.start_time,
        query.end_time,
        "search_queries.category1",
        "search_queries.category2",
        params);
  });

  queries_.registerQuery("catalog.CatalogCategoryE2DashboardQuery", [] (
      const cm::AnalyticsQuery& query,
      const cm::AnalyticsQuery::SubQueryParams params,
      const Vector<RefPtr<cm::TrafficSegment>>& segments,
      cm::AnalyticsTableScan* scan) {
    return new cm::CatalogCategoryDashboardQuery(
        scan,
        segments,
        query.start_time,
        query.end_time,
        "search_queries.category2",
        "search_queries.category3",
        params);
  });

  queries_.registerQuery("catalog.CatalogCategoryE3DashboardQuery", [] (
      const cm::AnalyticsQuery& query,
      const cm::AnalyticsQuery::SubQueryParams params,
      const Vector<RefPtr<cm::TrafficSegment>>& segments,
      cm::AnalyticsTableScan* scan) {
    return new cm::CatalogCategoryDashboardQuery(
        scan,
        segments,
        query.start_time,
        query.end_time,
        "search_queries.category3",
        "search_queries.category3",
        params);
  });

  queries_.registerQuery("search.TopSearchTermsQuery", [] (
      const cm::AnalyticsQuery& query,
      const cm::AnalyticsQuery::SubQueryParams params,
      const Vector<RefPtr<cm::TrafficSegment>>& segments,
      cm::AnalyticsTableScan* scan) {
    return new cm::TopSearchTermsQuery(scan, segments, params);
  });

  queries_.registerQuery("ecommerce.ECommerceDashboardQuery", [] (
      const cm::AnalyticsQuery& query,
      const cm::AnalyticsQuery::SubQueryParams params,
      const Vector<RefPtr<cm::TrafficSegment>>& segments,
      cm::AnalyticsTableScan* scan) {
    return new cm::ECommerceKPIQuery(
        scan,
        segments,
        query.start_time,
        query.end_time,
        params);
  });

  queries_.registerQuery("reco_engine.RecoEngineStatsQuery", [] (
      const cm::AnalyticsQuery& query,
      const cm::AnalyticsQuery::SubQueryParams params,
      const Vector<RefPtr<cm::TrafficSegment>>& segments,
      cm::AnalyticsTableScan* scan) {
    return new cm::RecoEngineStatsQuery(
        scan,
        segments,
        query.start_time,
        query.end_time,
        params);
  });

  queries_.registerQuery("reco_engine.RecoEngineStatsBreakdownQuery", [] (
      const cm::AnalyticsQuery& query,
      const cm::AnalyticsQuery::SubQueryParams params,
      const Vector<RefPtr<cm::TrafficSegment>>& segments,
      cm::AnalyticsTableScan* scan) {
    return new cm::RecoEngineStatsBreakdownQuery(
        scan,
        segments,
        query.start_time,
        query.end_time,
        params);
  });

  registerProtoRDDFactory<tsdb::TSDBTableScanSpec>(
      "reco_engine.ECommercePreferenceSetsFeed",
      [this] (const tsdb::TSDBTableScanSpec& params)
          -> RefPtr<dproc::Task> {
        auto report = new ECommercePreferenceSetsFeed(
            new TSDBTableScanSource<JoinedSession>(params, tsdb_node_),
            new JSONSink());

        return report;
      });


  registerProtoRDDFactory<tsdb::TSDBTableScanSpec>(
      "reco_engine.ECommerceRecoQueriesFeed",
      [this] (const tsdb::TSDBTableScanSpec& params)
          -> RefPtr<dproc::Task> {
        auto report = new ECommerceRecoQueriesFeed(
            new TSDBTableScanSource<JoinedSession>(params, tsdb_node_),
            new JSONSink());

        return report;
      });

  registerProtoRDDFactory<tsdb::TSDBTableScanSpec>(
      "search.ECommerceSearchQueriesFeed",
      [this] (const tsdb::TSDBTableScanSpec& params)
          -> RefPtr<dproc::Task> {
        auto report = new ECommerceSearchQueriesFeed(
            new TSDBTableScanSource<JoinedSession>(params, tsdb_node_),
            new JSONSink());

        return report;
      });

  registerProtoRDDFactory<ReportParams>(
      "search.TopSearchTermsReport",
      [this] (const ReportParams& params) -> RefPtr<dproc::Task> {
    return new TopSearchTermsReport(params);
  });

  registerProtoRDDFactory<ReportParams>(
      "search.TopSearchTermsViaGoogleReport",
      [this] (const ReportParams& params) -> RefPtr<dproc::Task> {
    return new TopSearchTermsReport(
        params,
        Some(String(R"(
          [
              {
                  "key": "google_traffic",
                  "name": "google traffic",
                  "rules": [
                      {
                          "field": "referrer_url",
                          "op": "includesi",
                          "values": [
                              "google"
                          ]
                      }
                  ]
              }
          ]
        )")));
  });

  registerProtoRDDFactory<tsdb::TSDBTableScanSpec>(
      "search.RelatedTermsMapper",
      [this] (const tsdb::TSDBTableScanSpec& params) -> RefPtr<dproc::Task> {
        auto report = new RelatedTermsMapper(
            new AnalyticsTableScanSource(params, tsdb_node_),
            new TermInfoTableSink());

        report->setCacheKey(
          StringUtil::format(
              "cm.analytics.search/relatedterms~$0~$1~$2~$3",
              params.tsdb_namespace(),
              params.table_name(),
              params.partition_key(),
              params.version()));

        return report;
      });

  registerProtoRDDFactory<ReportParams>(
      "search.RelatedTermsReport",
      [this] (const ReportParams& params) -> RefPtr<dproc::Task> {
        auto map_chunks = AnalyticsTableScanPlanner::mapShards(
            params.customer(),
            params.from_unixmicros(),
            params.until_unixmicros(),
            "search.RelatedTermsMapper",
            *msg::encode(params),
            tsdb_node_);

        return new RelatedTermsReport(
            new TermInfoTableSource(map_chunks),
            new CSVSink());
      });

  registerProtoRDDFactory<tsdb::TSDBTableScanSpec>(
      "search.TopTermsByCategoryMapper",
      [this] (const tsdb::TSDBTableScanSpec& params) -> RefPtr<dproc::Task> {
        auto rparams = msg::decode<ReportParams>(
            params.scanlet_params().data(),
            params.scanlet_params().size());

        URI::ParamList p;
        URI::parseQueryString(rparams.params(), &p);

        String category_field;
        if (!URI::getParam(p, "category_field", &category_field)) {
          RAISE(kRuntimeError, "missing ?category_field= parameter");
        }

        auto report = new CTRBySearchTermCrossCategoryMapper(
            new AnalyticsTableScanSource(params, tsdb_node_),
            new CTRCounterTableSink(),
            category_field);

        report->setCacheKey(
          StringUtil::format(
              "cm.analytics.toptermsbycategory~$0~$1~$2~$3~$4",
              category_field,
              params.tsdb_namespace(),
              params.table_name(),
              params.partition_key(),
              params.version()));

        return report;
      });

  registerProtoRDDFactory<ReportParams>(
      "search.TopTermsByCategoryReducer",
      [this] (const ReportParams& params) -> RefPtr<dproc::Task> {
        auto map_chunks = AnalyticsTableScanPlanner::mapShards(
            params.customer(),
            params.from_unixmicros(),
            params.until_unixmicros(),
            "search.TopTermsByCategoryMapper",
            *msg::encode(params),
            tsdb_node_);

        return new CTRCounterMergeReducer(
            new CTRCounterTableSource(map_chunks),
            new CTRCounterTableSink());
      });

  registerProtoRDDFactory<ReportParams>(
      "search.TopTermsByCategoryReport",
      [this] (const ReportParams& params) -> RefPtr<dproc::Task> {
        List<dproc::TaskDependency> deps;
        deps.emplace_back(dproc::TaskDependency {
          .task_name = "search.TopTermsByCategoryReducer",
          .params = *msg::encode(params)
        });

        return new TopTermsByCategoryReport(
            new CTRCounterTableSource(deps),
            new CSVSink());
      });

  registerProtoRDDFactory<ReportParams>(
      "search.TopCategoriesByTermMapper",
      [this] (const ReportParams& params) -> RefPtr<dproc::Task> {
        URI::ParamList p;
        URI::parseQueryString(params.params(), &p);

        String cat_prefix;
        URI::getParam(p, "cat_prefix", &cat_prefix);

        auto map_chunks = AnalyticsTableScanPlanner::mapShards(
            params.customer(),
            params.from_unixmicros(),
            params.until_unixmicros(),
            "search.TopTermsByCategoryMapper",
            *msg::encode(params),
            tsdb_node_);

        return new TopCategoriesByTermMapper(
            new CTRCounterTableSource(map_chunks),
            new TermInfoTableSink(),
            cat_prefix);
      });

  registerProtoRDDFactory<ReportParams>(
      "search.TopCategoriesByTermReport",
      [this] (const ReportParams& params) -> RefPtr<dproc::Task> {
        List<dproc::TaskDependency> deps;
        deps.emplace_back(dproc::TaskDependency {
          .task_name = "search.TopCategoriesByTermMapper",
          .params = *msg::encode(params)
        });

        return new TopCategoriesByTermReport(
            new TermInfoTableSource(deps),
            new CSVSink());
      });

  registerTaskFactory(
      "reco_engine.RecoEngineStatsReport",
      [this] (const Buffer& params) {
    return new RecoEngineStatsReport(msg::decode<ReportParams>(params));
  });

  registerProtoRDDFactory<tsdb::TSDBTableScanSpec>(
      "shop_stats.ShopCTRStatsScan",
      [this] (const tsdb::TSDBTableScanSpec& params) -> RefPtr<dproc::Task> {
    return new ShopCTRStatsScan(
            new TSDBTableScanSource<JoinedSession>(params, tsdb_node_),
            new ProtoSSTableSink<ShopKPIs>(),
            msg::decode<ReportParams>(
                params.scanlet_params().data(),
                params.scanlet_params().size()));
  });

  registerProtoRDDFactory<tsdb::TSDBTableScanSpec>(
      "shop_stats.ShopECommerceStatsScan",
      [this] (const tsdb::TSDBTableScanSpec& params) -> RefPtr<dproc::Task> {
    return new ShopECommerceStatsScan(
            new TSDBTableScanSource<ECommerceTransaction>(params, tsdb_node_),
            new ProtoSSTableSink<ShopKPIs>(),
            msg::decode<ReportParams>(
                params.scanlet_params().data(),
                params.scanlet_params().size()));
  });

  registerProtoRDDFactory<tsdb::TSDBTableScanSpec>(
      "shop_stats.ShopProductCTRStatsScan",
      [this] (const tsdb::TSDBTableScanSpec& params) -> RefPtr<dproc::Task> {
    return new ShopProductCTRStatsScan(
            new TSDBTableScanSource<JoinedSession>(params, tsdb_node_),
            new ProtoSSTableSink<ShopProductKPIs>(),
            msg::decode<ReportParams>(
                params.scanlet_params().data(),
                params.scanlet_params().size()));
  });

  registerProtoRDDFactory<tsdb::TSDBTableScanSpec>(
      "shop_stats.ShopProductECommerceStatsScan",
      [this] (const tsdb::TSDBTableScanSpec& params) -> RefPtr<dproc::Task> {
    return new ShopProductECommerceStatsScan(
            new TSDBTableScanSource<ECommerceTransaction>(params, tsdb_node_),
            new ProtoSSTableSink<ShopProductKPIs>(),
            msg::decode<ReportParams>(
                params.scanlet_params().data(),
                params.scanlet_params().size()));
  });

  registerProtoRDDFactory<ReportParams>(
      "shop_stats.ShopKPITableCSV",
      [this] (const ReportParams& params) -> RefPtr<dproc::Task> {
    return new CSVExportRDD(new ShopKPITable(params, tsdb_node_));
  });

  registerProtoRDDFactory<ReportParams>(
      "shop_stats.ShopKPITableJSON",
      [this] (const ReportParams& params) -> RefPtr<dproc::Task> {
    return new JSONExportRDD(new ShopKPITable(params, tsdb_node_));
  });

  registerProtoRDDFactory<ReportParams>(
      "shop_stats.ShopKPIDashboardQuery",
      [this] (const ReportParams& params) -> RefPtr<dproc::Task> {
    return new ShopKPIDashboardQuery(params, tsdb_node_);
  });

  registerProtoRDDFactory<ReportParams>(
      "shop_stats.ShopProductsTableCSV",
      [this] (const ReportParams& params) -> RefPtr<dproc::Task> {
    return new CSVExportRDD(new ShopProductsTable(params, tsdb_node_));
  });

  registerProtoRDDFactory<ReportParams>(
      "shop_stats.ShoProductsTableJSON",
      [this] (const ReportParams& params) -> RefPtr<dproc::Task> {
    return new JSONExportRDD(new ShopProductsTable(params, tsdb_node_));
  });

  registerProtoRDDFactory<ReportParams>(
      "shop_stats.ShopProductsDashboardQuery",
      [this] (const ReportParams& params) -> RefPtr<dproc::Task> {
    return new ShopProductsDashboardQuery(params, tsdb_node_);
  });

  registerProtoRDDFactory<tsdb::TSDBTableScanSpec>(
      "AnalyticsQueryMapper",
      [this] (const tsdb::TSDBTableScanSpec& params) -> RefPtr<dproc::Task> {
    return new AnalyticsQueryMapper(params, partition_map_, &queries_);
  });

  registerProtoRDDFactory<AnalyticsQuerySpec>(
      "AnalyticsQueryReducer",
      [this] (const AnalyticsQuerySpec& params) -> RefPtr<dproc::Task> {
    return new AnalyticsQueryReducer(params, tsdb_node_, &queries_);
  });
}

void AnalyticsApp::configureFeed(const FeedConfig& cfg) {
  feeds_.emplace(
      StringUtil::format("$0~$1", cfg.feed(), cfg.customer()),
      cfg);
}

dproc::TaskSpec AnalyticsApp::buildAnalyticsQuery(
    const AnalyticsSession& session,
    const AnalyticsQuerySpec& query_spec) {
  dproc::TaskSpec task;
  task.set_application("cm.analytics");
  task.set_task_name("AnalyticsQueryReducer");
  auto task_params = msg::encode(query_spec);
  task.set_params((char*) task_params->data(), task_params->size());
  return task;
}

dproc::TaskSpec AnalyticsApp::buildAnalyticsQuery(
    const AnalyticsSession& session,
    const URI::ParamList& params) {
  AnalyticsQuerySpec q;
  AnalyticsSubQuerySpec* subq = nullptr;

  q.set_customer(session.customer());
  q.set_end_time(WallClock::unixMicros());
  q.set_start_time(q.end_time() - 30 * kMicrosPerDay);

  for (const auto& p : params) {
    if (p.first == "from") {
      if (p.second.length() > 0) {
        q.set_start_time(std::stoul(p.second) * kMicrosPerSecond);
      }
      continue;
    }

    if (p.first == "until") {
      if (p.second.length() > 0) {
        q.set_end_time(std::stoul(p.second) * kMicrosPerSecond);
      }
      continue;
    }

    if (p.first == "query") {
      subq = q.add_subqueries();
      subq->set_query(p.second);
      continue;
    }

    if (p.first == "segments") {
      if (p.second.length() == 0) {
        continue;
      }

      String segments_json;
      util::Base64::decode(p.second, &segments_json);
      q.set_segments(segments_json);
      continue;
    }

    if (subq != nullptr) {
      auto param = subq->add_params();
      param->set_key(p.first);
      param->set_value(p.second);
    }
  }

  q.set_start_time((q.start_time() / kMicrosPerDay) * kMicrosPerDay);
  q.set_end_time(((q.end_time() / kMicrosPerDay) + 1) * kMicrosPerDay);

  return buildAnalyticsQuery(session, q);
}

dproc::TaskSpec AnalyticsApp::buildReportQuery(
      const String& customer,
      const String& report_name,
      const UnixTime& from,
      const UnixTime& until,
      const URI::ParamList& params) {
  ReportParams report;
  report.set_report(report_name);
  report.set_customer(customer);
  report.set_from_unixmicros(from.unixMicros());
  report.set_until_unixmicros(until.unixMicros());
  report.set_params(URI::buildQueryString(params));

  dproc::TaskSpec task;
  task.set_application("cm.analytics");
  task.set_task_name(report_name);
  task.set_params(msg::encode(report)->toString());

  return task;
}

dproc::TaskSpec AnalyticsApp::buildFeedQuery(
      const String& customer,
      const String& feed,
      uint64_t sequence) {
  RAISE(kNotImplementedError);

  //auto cfg_iter = feeds_.find(
  //    StringUtil::format(
  //        "$0~$1",
  //        feed,
  //        customer));

  //if (cfg_iter == feeds_.end()) {
  //  RAISEF(
  //      kRuntimeError,
  //      "feed '$0' not configured for customer '$1'",
  //      feed,
  //      customer);
  //}

  //const auto& cfg = cfg_iter->second;
  //auto ts =
  //    cfg.first_partition() +
  //    cfg.partition_size() * (sequence / cfg.num_shards());

  //auto table_name = cfg.table_name();
  //auto partition_key = tsdb::TimeWindowPartitioner::partitionKeyFor(
  //    table_name,
  //    ts,
  //    cfg.partition_size());

  //tsdb::TSDBTableScanSpec params;
  //params.set_tsdb_namespace(customer);
  //params.set_table_name(table_name);
  //params.set_partition_sha1(partition_key.toString());
  //params.set_sample_modulo(cfg.num_shards());
  //params.set_sample_index(sequence % cfg.num_shards());

  //auto params_buf = msg::encode(params);

  //dproc::TaskSpec task;
  //task.set_application(name());
  //task.set_task_name(feed);
  //task.set_params((char*) params_buf->data(), params_buf->size());

  //return task;
}

RefPtr<csql::ExecutionStrategy> AnalyticsApp::getExecutionStrategy(
    const String& customer) {
  auto strategy = mkRef(new csql::DefaultExecutionStrategy());

  strategy->addTableProvider(
      tsdb::SQLEngine::tableProviderForNamespace(
          partition_map_,
          cstable_index_,
          customer));

  strategy->addQueryTreeRewriteRule(
      std::bind(
          &tsdb::SQLEngine::rewriteQuery,
          partition_map_,
          cstable_index_,
          customer,
          std::placeholders::_1));

  return strategy.get();
}

void AnalyticsApp::createTable(const TableDefinition& tbl) {
  cdb_->updateTableDefinition(tbl);
}

void AnalyticsApp::updateTable(const TableDefinition& tbl, bool force) {
  cdb_->updateTableDefinition(tbl, force);
}

void AnalyticsApp::configureTable(const TableDefinition& tbl) {
  tsdb::TableDefinition td;
  td.set_tsdb_namespace(tbl.customer());
  td.set_table_name(tbl.table_name());
  *td.mutable_config() = tbl.config();
  tsdb_node_->createTable(td);
}

void AnalyticsApp::configureCustomer(const CustomerConfig& config) {
  for (const auto& td : SessionSchema::tableDefinitionsForCustomer(config)) {
    configureTable(td);
  }

  for (const auto& td : logfile_service_.getTableDefinitions(config)) {
    configureTable(td);
  }
}

void AnalyticsApp::insertMetric(
    const String& customer,
    const String& metric,
    const UnixTime& time,
    const String& value) {
  RefPtr<tsdb::Table> table;
  auto table_opt = partition_map_->findTable(customer, metric);
  if (table_opt.isEmpty()) {
    auto schema = msg::MessageSchema(
        "generic_metric",
        Vector<msg::MessageSchemaField> {
            msg::MessageSchemaField(
                1,
                "time",
                msg::FieldType::DATETIME,
                0,
                false,
                true),
            msg::MessageSchemaField(
                2,
                "value",
                msg::FieldType::DOUBLE,
                0,
                false,
                true),
        });

    TableDefinition td;
    td.set_customer(customer);
    td.set_table_name(metric);
    auto tblcfg = td.mutable_config();
    tblcfg->set_schema(schema.encode().toString());
    tblcfg->set_partitioner(tsdb::TBL_PARTITION_TIMEWINDOW);
    tblcfg->set_storage(tsdb::TBL_STORAGE_LOG);
    createTable(td);
    table = partition_map_->findTable(customer, metric).get();
  } else {
    table = table_opt.get();
  }

  msg::DynamicMessage smpl(table->schema());
  smpl.addField("time", StringUtil::toString(time));
  smpl.addField("value", value);

  Buffer smpl_buf;
  msg::MessageEncoder::encode(smpl.data(), *smpl.schema(), &smpl_buf);

  tsdb_node_->insertRecord(
      customer,
      metric,
      tsdb::TimeWindowPartitioner::partitionKeyFor(
          metric,
          time,
          table->partitionSize()),
      Random::singleton()->sha1(),
      smpl_buf);
}

LogfileService* AnalyticsApp::logfileService() {
  return &logfile_service_;
}

} // namespace cm
