include_directories(${CMAKE_CURRENT_BINARY_DIR}/)

file(GLOB PROTO_FILES *.proto)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

set(ZBASE_ASSETS_PACK "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/zbase_assets.cc")
if(EXISTS ${ZBASE_ASSETS_PACK})
  set(ZBASE_ASSETS_SRCS ${ZBASE_ASSETS_SRCS} ${ZBASE_ASSETS_PACK})
  set(ZBASE_HAS_ASSET_BUNDLE TRUE)
else()
  if (NOT ZBASE_BUILD_ID MATCHES "devel")
    message(FATAL_ERROR "ZBase release build with dynamic assets is not allowed")
  endif()
endif()

set(ZBASE_DOCASSETS_PACK "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/zbase_documentation_assets.cc")
if(EXISTS ${ZBASE_DOCASSETS_PACK})
  set(ZBASE_ASSETS_SRCS ${ZBASE_ASSETS_SRCS} ${ZBASE_DOCASSETS_PACK})
else()
  if (NOT ZBASE_BUILD_ID MATCHES "devel")
    message(FATAL_ERROR "ZBase release build without documentation assets is not allowed")
  endif()
endif()

add_library(customerdirectory OBJECT
    CustomerConfig.cc
    ConfigDirectory.cc
    SessionSchema.cc
    TableDefinition.cc
    ${PROTO_SRCS}
    ${ZBASE_ASSETS_SRCS})

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/buildconfig.h.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/buildconfig.h)

add_dependencies(customerdirectory tsdb-server)

add_library(auth OBJECT
    AnalyticsAuth.cc)

add_dependencies(auth customerdirectory)

file(GLOB COREPROTO_FILES core/*.proto)
PROTOBUF_GENERATE_CPP(COREPROTO_SRCS COREPROTO_HDRS ${COREPROTO_FILES})

add_library(tsdb-client OBJECT
    core/TSDBClient.cc
    core/TimeWindowPartitioner.cc
    core/FixedShardPartitioner.cc
    ${COREPROTO_SRCS})

add_library(tsdb-server OBJECT
    core/TSDBClient.cc
    core/TimeWindowPartitioner.cc
    core/FixedShardPartitioner.cc
    core/CSTableIndex.cc
    core/SQLEngine.cc
    core/TSDBService.cc
    core/TSDBServlet.cc
    core/TSDBTableProvider.cc
    core/TSDBTableRef.cc
    core/RecordSet.cc
    core/Partition.cc
    core/PartitionMap.cc
    core/PartitionReader.cc
    core/PartitionWriter.cc
    core/PartitionSnapshot.cc
    core/PartitionReplication.cc
    core/LogPartitionWriter.cc
    core/LogPartitionReplication.cc
    core/LazyPartition.cc
    core/Table.cc
    core/ReplicationWorker.cc
    core/ReplicationScheme.cc
    core/ReplicationState.cc
    core/RemoteTSDBScan.cc
    core/StaticPartitionWriter.cc
    core/StaticPartitionReplication.cc
    ${COREPROTO_SRCS})

add_executable(test-recordset
    $<TARGET_OBJECTS:stx-mdb>
    $<TARGET_OBJECTS:tsdb-server>
    $<TARGET_OBJECTS:dproc>
    $<TARGET_OBJECTS:auth>
    $<TARGET_OBJECTS:customerdirectory>
    core/RecordSet_test.cc)

target_link_libraries(test-recordset chartsql cplot sstable cstable stx-http stx-protobuf stx-json stx-base)

add_executable(test-tsdb-timewindowpartitioner
    $<TARGET_OBJECTS:stx-mdb>
    $<TARGET_OBJECTS:tsdb-server>
    $<TARGET_OBJECTS:dproc>
    $<TARGET_OBJECTS:auth>
    $<TARGET_OBJECTS:customerdirectory>
    core/TimeWindowPartitioner_test.cc)

target_link_libraries(test-tsdb-timewindowpartitioner chartsql cplot sstable cstable stx-http stx-protobuf stx-json stx-base)

add_subdirectory(util)
add_subdirectory(util/mysql)
add_subdirectory(util/mdb)

add_executable(zbase
    $<TARGET_OBJECTS:stx-mdb>
    $<TARGET_OBJECTS:stx-rpc>
    $<TARGET_OBJECTS:brokerd-client>
    $<TARGET_OBJECTS:logjoin-common>
    $<TARGET_OBJECTS:dxa-docdb>
    $<TARGET_OBJECTS:tsdb-server>
    $<TARGET_OBJECTS:customerdirectory>
    #$<TARGET_OBJECTS:fnord-afx>
    $<TARGET_OBJECTS:dproc>
    $<TARGET_OBJECTS:auth>
    $<TARGET_OBJECTS:pcre>
    #sstable/SSTableServlet.cc
    api/LogfileService.cc
    api/LogfileAPIServlet.cc
    api/EventsService.cc
    api/EventsAPIServlet.cc
    LogfileScanResult.cc
    EventScanResult.cc
    Report.cc
    AnalyticsTableScan.cc
    AnalyticsTableScanPlanner.cc
    TrafficSegment.cc
    CTRStatsServlet.cc
    AnalyticsServlet.cc
    HTTPAuth.cc
    JSONSink.cc
    AnalyticsApp.cc
    AnalyticsQuery.cc
    AnalyticsQueryResult.cc
    AnalyticsQueryFactory.cc
    AnalyticsQueryMapper.cc
    AnalyticsQueryReducer.cc
    AnalyticsTableScanSource.cc
    DrilldownTree.cc
    EventIngress.cc
    ReportFactory.cc
    CSVExportRDD.cc
    JSONExportRDD.cc
    CSVSink.cc
    CTRCounter.cc
    CTRCounterTableSink.cc
    CTRCounterTableSource.cc
    CTRCounterMergeReducer.cc
    PipelineInfo.cc
    TermInfoTableSource.cc
    TermInfoTableSink.cc
    VTable.cc
    DefaultServlet.cc
    ecommerce/ECommerceKPIQuery.cc
    discovery/CTRByPageQuery.cc
    discovery/CTRByPositionQuery.cc
    discovery/DiscoveryDashboardQuery.cc
    discovery/ItemBoostMapper.cc
    discovery/ItemBoostMerge.cc
    discovery/ItemBoostExport.cc
    search/CTRByResultItemCategoryQuery.cc
    search/TopSearchTermsQuery.cc
    search/TopCategoriesByTermMapper.cc
    search/SearchCTRStats.cc
    search/SearchDashboardQuery.cc
    search/ECommerceSearchQueriesFeed.cc
    search/TopSearchTermsReport.cc
    search/TopTermsByCategoryReport.cc
    search/TopCategoriesByTermReport.cc
    search/CTRBySearchTermCrossCategoryMapper.cc
    search/RelatedTermsMapper.cc
    search/RelatedTermsReport.cc
    catalog/CatalogDashboardQuery.cc
    catalog/CatalogCategoryDashboardQuery.cc
    catalog/CatalogCategoryDashboardResult.cc
    shop_stats/ShopStats.cc
    shop_stats/ShopProductsTable.cc
    shop_stats/ShopProductCTRStatsScan.cc
    shop_stats/ShopProductECommerceStatsScan.cc
    shop_stats/ShopKPITable.cc
    shop_stats/ShopCTRStatsScan.cc
    shop_stats/ShopECommerceStatsScan.cc
    shop_stats/ShopKPIDashboardQuery.cc
    shop_stats/ShopProductsDashboardQuery.cc
    reco_engine/ECommerceRecoQueriesFeed.cc
    reco_engine/ECommercePreferenceSetsFeed.cc
    reco_engine/RecoEngineStatsQuery.cc
    reco_engine/RecoEngineStatsBreakdownQuery.cc
    reco_engine/RecoEngineStatsReport.cc
    reco_engine/RecoStats.cc
    WebUIServlet.cc
    zbase.cc)

target_link_libraries(zbase chartsql cplot sstable cstable stx-http stx-json stx-protobuf stx-base)

add_executable(zbasectl
    $<TARGET_OBJECTS:stx-mdb>
    $<TARGET_OBJECTS:stx-rpc>
    $<TARGET_OBJECTS:brokerd-client>
    $<TARGET_OBJECTS:logjoin-common>
    $<TARGET_OBJECTS:tsdb-server>
    $<TARGET_OBJECTS:customerdirectory>
    #$<TARGET_OBJECTS:fnord-afx>
    $<TARGET_OBJECTS:dproc>
    $<TARGET_OBJECTS:auth>
    #sstable/SSTableServlet.cc
    zbasectl.cc)

target_link_libraries(zbasectl chartsql cplot sstable cstable stx-http stx-json stx-protobuf stx-base)

add_executable(zmaster
    $<TARGET_OBJECTS:stx-mdb>
    $<TARGET_OBJECTS:stx-rpc>
    $<TARGET_OBJECTS:customerdirectory>
    $<TARGET_OBJECTS:tsdb-server>
    $<TARGET_OBJECTS:auth>
    master/ConfigDirectoryMaster.cc
    master/MasterServlet.cc
    zmaster.cc)

target_link_libraries(zmaster chartsql cplot sstable cstable stx-http stx-protobuf stx-json stx-json stx-base)

add_executable(zen-csv-upload ${COREPROTO_SRCS} zen-csv-upload.cc)
target_link_libraries(zen-csv-upload stx-http stx-protobuf stx-json stx-base)

add_executable(zen-logfile-upload ${COREPROTO_SRCS} zen-logfile-upload.cc)
target_link_libraries(zen-logfile-upload stx-http stx-protobuf stx-json stx-base)

add_executable(zen-statsd-upload ${COREPROTO_SRCS} zen-statsd-upload.cc)
target_link_libraries(zen-statsd-upload stx-http stx-protobuf stx-json stx-base)

if(MYSQL_FOUND)
  add_executable(zen-mysql-upload
      $<TARGET_OBJECTS:stx-mysql>
      $<TARGET_OBJECTS:tsdb-client>
      ${PROTO_SRCS}
      zen-mysql-upload.cc)

  target_link_libraries(zen-mysql-upload stx-http stx-protobuf stx-json stx-base ${MYSQL_CLIENT_LIBS} z)
endif()

add_subdirectory(tracker)
