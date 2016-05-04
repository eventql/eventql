include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_BINARY_DIR}/3rdparty/libfnord/fnord-msg)

file(GLOB PROTO_FILES *.proto)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

add_library(cm-common OBJECT
    common.cc
    CTRCounter.cc
    Document.cc
    ItemRef.cc
    CustomerNamespace.cc
    ${PROTO_SRCS}
    ${PROTO_HDRS})

# cm-frontend
set_source_files_properties(${CMAKE_BINARY_DIR}/cm-frontend-assets.cc PROPERTIES GENERATED true)

execute_process(
    COMMAND bash -c "sed -e 's/.*://' < ${CMAKE_SOURCE_DIR}/src/cm-frontend.assets | tr '\n' ' '"
    OUTPUT_VARIABLE FRONTEND_ASSETS_DEPS
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

separate_arguments(FRONTEND_ASSETS_DEPS)
foreach(l ${FRONTEND_ASSETS_DEPS})
  list(APPEND FRONTEND_ASSETS_DEPS_FULL ${CMAKE_SOURCE_DIR}/${l})
endforeach()

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/cm-frontend-assets.cc
    COMMAND ./3rdparty/libfnord/stx-base/assets.sh ${CMAKE_BINARY_DIR}/cm-frontend-assets.cc < ./src/cm-frontend.assets
    DEPENDS ${FRONTEND_ASSETS_DEPS_FULL}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(cm-frontend
    $<TARGET_OBJECTS:cm-common>
    
    
    
    $<TARGET_OBJECTS:stx-mdb>
    
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:stx-rpc>
    
    frontend/CMFrontend.cc
    frontend/IndexFeedUpload.cc
    cm-frontend.cc
    schemas.cc
    ${CMAKE_BINARY_DIR}/cm-frontend-assets.cc)

target_link_libraries(cm-frontend ${CMAKE_THREAD_LIBS_INIT} ${FNORD_LIBS})
target_link_libraries(cm-frontend m)
add_dependencies(cm-frontend cm-frontend-assets)

#add_executable(cm-feedserver
#    $<TARGET_OBJECTS:cm-common>
#    
#    
#    $<TARGET_OBJECTS:stx-mdb>
#    
#    
#    $<TARGET_OBJECTS:fnord-feeds>
#    $<TARGET_OBJECTS:stx-rpc>
#    cm-feedserver.cc)
#
#target_link_libraries(cm-feedserver ${CMAKE_THREAD_LIBS_INIT} ${FNORD_LIBS})

add_executable(cm-logjoin
    $<TARGET_OBJECTS:cm-common>
    
    
    
    
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:stx-mdb>
    $<TARGET_OBJECTS:stx-rpc>
    
    $<TARGET_OBJECTS:nlputil>
    logjoin/TrackedCartItem.cc
    logjoin/TrackedItemVisit.cc
    logjoin/TrackedQuery.cc
    logjoin/TrackedSession.cc
    DocIndex.cc
    cm-logjoin.cc
    schemas.cc
    logjoin/LogJoin.cc
    logjoin/LogJoinUpload.cc
    logjoin/LogJoinExport.cc
    logjoin/SessionProcessor.cc
    logjoin/LogJoinShard.cc)

target_link_libraries(cm-logjoin ${CMAKE_THREAD_LIBS_INIT} ${FNORD_LIBS} ${FNORD_FTS_LIBS})

add_executable(tests/test-logjoin
    $<TARGET_OBJECTS:cm-common>
    
    
    
    
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:stx-mdb>
    $<TARGET_OBJECTS:stx-rpc>
    
    $<TARGET_OBJECTS:nlputil>
    logjoin/TrackedCartItem.cc
    logjoin/TrackedItemVisit.cc
    logjoin/TrackedQuery.cc
    logjoin/TrackedSession.cc
    LogJoin_test.cc)

target_link_libraries(tests/test-logjoin ${CMAKE_THREAD_LIBS_INIT} ${FNORD_LIBS} ${FNORD_FTS_LIBS})

#add_executable(cm-logjoin-backfill
#    $<TARGET_OBJECTS:cm-common>
#    
#    $<TARGET_OBJECTS:fnord-feeds>
#    $<TARGET_OBJECTS:stx-mdb>
#    
#    
#    $<TARGET_OBJECTS:stx-rpc>
#    
#    
#    $<TARGET_OBJECTS:nlputil>
#    
#    3rdparty/libfnord/fnord-logtable/TableReader.cc
#    3rdparty/libfnord/fnord-logtable/TableArena.cc
#    3rdparty/libfnord/fnord-logtable/TableWriter.cc
#    3rdparty/libfnord/fnord-logtable/TableRepository.cc
#    3rdparty/libfnord/fnord-logtable/ArtifactIndex.cc
#    3rdparty/libfnord/fnord-logtable/NumericBoundsSummary.cc
#    3rdparty/libfnord/fnord-logtable/TableChunkSummaryReader.cc
#    3rdparty/libfnord/fnord-logtable/TableChunkSummaryWriter.cc
#    3rdparty/libfnord/fnord-logtable/LogTableTail.cc
#    logjoin/LogJoinBackfill.cc
#    schemas.cc
#    DocIndex.cc
#    cm-logjoin-backfill.cc)
#
#target_link_libraries(cm-logjoin-backfill
#    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_LIBS} ${FNORD_FTS_LIBS})
#
#add_executable(cm-indexbuild
#    $<TARGET_OBJECTS:cm-common>
#    
#    
#    
#    
#    $<TARGET_OBJECTS:nlputil>
#    $<TARGET_OBJECTS:fnord-feeds>
#    $<TARGET_OBJECTS:stx-rpc>
#    $<TARGET_OBJECTS:stx-mdb>
#    
#    
#    $<TARGET_OBJECTS:fnord-afx>
#    cm-indexbuild.cc
#    ../3rdparty/libfnord/fnord-logtable/RemoteTableReader.cc
#    ../3rdparty/libfnord/fnord-logtable/TableReader.cc
#    ../3rdparty/libfnord/fnord-logtable/TableArena.cc
#    ../3rdparty/libfnord/fnord-logtable/TableWriter.cc
#    ../3rdparty/libfnord/fnord-logtable/TableRepository.cc
#    ../3rdparty/libfnord/fnord-logtable/NumericBoundsSummary.cc
#    ../3rdparty/libfnord/fnord-logtable/TableChunkSummaryReader.cc
#    ../3rdparty/libfnord/fnord-logtable/TableChunkSummaryWriter.cc
#    ../3rdparty/libfnord/fnord-logtable/LogTableTail.cc
#    schemas.cc
#    DocStore.cc
#    DocIndex.cc)
#
#target_link_libraries(cm-indexbuild
#    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_LIBS} ${FNORD_FTS_LIBS})

#add_executable(cm-indexlookup
#    $<TARGET_OBJECTS:cm-common>
#    
#    
#    
#    
#    $<TARGET_OBJECTS:nlputil>
#    $<TARGET_OBJECTS:fnord-feeds>
#    $<TARGET_OBJECTS:stx-rpc>
#    $<TARGET_OBJECTS:stx-mdb>
#    DocStore.cc
#    DocIndex.cc
#    cm-indexlookup.cc)
#
#target_link_libraries(cm-indexlookup
#    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_LIBS}
#    ${FNORD_FTS_LIBS})
#
#add_executable(cm-indexserver
#    $<TARGET_OBJECTS:cm-common>
#    
#    
#    
#    
#    $<TARGET_OBJECTS:nlputil>
#    $<TARGET_OBJECTS:fnord-feeds>
#    $<TARGET_OBJECTS:stx-rpc>
#    $<TARGET_OBJECTS:stx-mdb>
#    DocStore.cc
#    IndexServlet.cc
#    SearchQuery.cc
#    DocIndex.cc
#    cm-indexserver.cc)
#
#target_link_libraries(cm-indexserver
#    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_LIBS}
#    ${FNORD_FTS_LIBS})

#add_executable(cm-dawandaindexpush
#    $<TARGET_OBJECTS:cm-common>
#    
#    
#    
#    $<TARGET_OBJECTS:stx-rpc>
#    $<TARGET_OBJECTS:stx-mdb>
#    $<TARGET_OBJECTS:fnord-sql>
#    
#    customers/dawanda/cm-dawandaindexpush.cc)
#
#target_link_libraries(cm-dawandaindexpush
#    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_LIBS})

#add_executable(cm-featuredump
#    $<TARGET_OBJECTS:cm-common>
#    
#    $<TARGET_OBJECTS:fnord-feeds>
#    
#    $<TARGET_OBJECTS:stx-mdb>
#    
#    $<TARGET_OBJECTS:stx-rpc>
#    
#    cm-featuredump.cc)
#
#target_link_libraries(cm-featuredump
#    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_LIBS})
#
#add_executable(cm-featureprep
#    $<TARGET_OBJECTS:cm-common>
#    
#    $<TARGET_OBJECTS:fnord-feeds>
#    $<TARGET_OBJECTS:stx-mdb>
#    
#    
#    $<TARGET_OBJECTS:stx-rpc>
#    
#    $<TARGET_OBJECTS:nlputil>
#    FeatureSelector.cc
#    cm-featureprep.cc)
#
#target_link_libraries(cm-featureprep
#    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_LIBS} ${FNORD_FTS_LIBS})

#add_executable(cm-ctrstats-builditemboost
#    $<TARGET_OBJECTS:cm-common>
#    
#    $<TARGET_OBJECTS:fnord-feeds>
#    $<TARGET_OBJECTS:stx-mdb>
#    
#    
#    $<TARGET_OBJECTS:stx-rpc>
#    
#    $<TARGET_OBJECTS:nlputil>
#    cm-ctrstats-builditemboost.cc)
#
#target_link_libraries(cm-ctrstats-builditemboost
#    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_LIBS} ${FNORD_FTS_LIBS})

#add_executable(cm-reportbuild
#    $<TARGET_OBJECTS:cm-common>
#    
#    $<TARGET_OBJECTS:fnord-feeds>
#    $<TARGET_OBJECTS:stx-mdb>
#    
#    
#    $<TARGET_OBJECTS:stx-rpc>
#    
#    $<TARGET_OBJECTS:nlputil>
#    IndexReader.cc
#    IndexWriter.cc
#    DocIndex.cc
#    analytics/Report.cc
#    analytics/ReportBuilder.cc
#    analytics/CTRByPageMapper.cc
#    analytics/CTRStatsMapper.cc
#    analytics/CTRCounterTableSink.cc
#    analytics/CTRCounterTableSource.cc
#    analytics/CTRCounterMergeReducer.cc
#    analytics/CTRBySearchTermCrossCategoryMapper.cc
#    analytics/CTRByQueryAttributeMapper.cc
#    analytics/RelatedTermsMapper.cc
#    analytics/TermInfoTableSink.cc
#    analytics/TermInfoTableSource.cc
#    analytics/TermInfoMergeReducer.cc
#    analytics/TermInfo.cc
#    analytics/TopCategoriesByTermMapper.cc
#    cm-reportbuild.cc)
#
#target_link_libraries(cm-reportbuild
#    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_LIBS} ${FNORD_FTS_LIBS})

#add_executable(cm-autocompletebuild
#    $<TARGET_OBJECTS:cm-common>
#    
#    $<TARGET_OBJECTS:fnord-feeds>
#    $<TARGET_OBJECTS:stx-mdb>
#    
#    
#    $<TARGET_OBJECTS:stx-rpc>
#    
#    
#    $<TARGET_OBJECTS:nlputil>
#    
#    3rdparty/libfnord/fnord-logtable/TableReader.cc
#    3rdparty/libfnord/fnord-logtable/TableArena.cc
#    3rdparty/libfnord/fnord-logtable/TableWriter.cc
#    3rdparty/libfnord/fnord-logtable/TableRepository.cc
#    3rdparty/libfnord/fnord-logtable/ArtifactIndex.cc
#    3rdparty/libfnord/fnord-logtable/NumericBoundsSummary.cc
#    3rdparty/libfnord/fnord-logtable/TableChunkSummaryReader.cc
#    3rdparty/libfnord/fnord-logtable/TableChunkSummaryWriter.cc
#    schemas.cc
#    DocIndex.cc
#    analytics/Report.cc
#    analytics/ReportBuilder.cc
#    analytics/CTRCounterTableSink.cc
#    analytics/CTRCounterTableSource.cc
#    analytics/CTRCounterMergeReducer.cc
#    analytics/TrafficSegment.cc
#    analytics/AnalyticsTableScan.cc
#    analytics/AnalyticsQuery.cc
#    analytics/AnalyticsQueryEngine.cc
#    analytics/AnalyticsQueryResult.cc
#    analytics/CTRBySearchTermCrossCategoryMapper.cc
#    analytics/AnalyticsTableScanSource.cc
#    analytics/RelatedTermsMapper.cc
#    analytics/TermInfoTableSink.cc
#    analytics/TermInfoTableSource.cc
#    analytics/TermInfoMergeReducer.cc
#    analytics/TermInfo.cc
#    analytics/TopCategoriesByTermMapper.cc
#    cm-autocompletebuild.cc)
#
#target_link_libraries(cm-autocompletebuild
#    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_LIBS} ${FNORD_FTS_LIBS})

# add_executable(cm-shopstatsbuild
#     $<TARGET_OBJECTS:cm-common>
#     
#     $<TARGET_OBJECTS:fnord-feeds>
#     $<TARGET_OBJECTS:stx-mdb>
#     
#     
#     $<TARGET_OBJECTS:stx-rpc>
#     
#     
#     $<TARGET_OBJECTS:nlputil>
#     
#     $<TARGET_OBJECTS:tsdb-client>
#     $<TARGET_OBJECTS:dproc>
#     $<TARGET_OBJECTS:fnord-afx>
#     ../3rdparty/libfnord/fnord-logtable/TableReader.cc
#     ../3rdparty/libfnord/fnord-logtable/TableArena.cc
#     ../3rdparty/libfnord/fnord-logtable/TableWriter.cc
#     ../3rdparty/libfnord/fnord-logtable/TableRepository.cc
#     ../3rdparty/libfnord/fnord-logtable/NumericBoundsSummary.cc
#     ../3rdparty/libfnord/fnord-logtable/TableChunkSummaryReader.cc
#     ../3rdparty/libfnord/fnord-logtable/TableChunkSummaryWriter.cc
#     schemas.cc
#     DocIndex.cc
#     analytics/Report.cc
#     analytics/CTRByShopMapper.cc
#     analytics/ECommerceStatsByShopMapper.cc
#     analytics/ProductStatsByShopMapper.cc
#     analytics/TrafficSegment.cc
#     analytics/AnalyticsTableScan.cc
#     analytics/AnalyticsQuery.cc
#     analytics/AnalyticsQueryEngine.cc
#     analytics/AnalyticsQueryResult.cc
#     analytics/AnalyticsTableScanSource.cc
#     analytics/ShopStats.cc
#     analytics/ShopStatsApp.cc
#     analytics/CTRCounterTableSource.cc
#     analytics/CTRCounterTableSink.cc
#     analytics/CTRCounterMergeReducer.cc
#     analytics/CTRBySearchTermCrossCategoryMapper.cc
#     analytics/TopTermsByCategoryReport.cc
#     cm-shopstatsbuild.cc
#     ${PROTO_SRCS})
# 
# target_link_libraries(cm-shopstatsbuild
#     ${CMAKE_THREAD_LIBS_INIT} ${FNORD_LIBS} ${FNORD_FTS_LIBS})

# add_executable(cm-itemboostbuild
#     $<TARGET_OBJECTS:cm-common>
#     
#     $<TARGET_OBJECTS:fnord-feeds>
#     $<TARGET_OBJECTS:stx-mdb>
#     
#     
#     $<TARGET_OBJECTS:stx-rpc>
#     
#     
#     $<TARGET_OBJECTS:nlputil>
#     
#     $<TARGET_OBJECTS:tsdb-client>
#     $<TARGET_OBJECTS:dproc>
#     schemas.cc
#     DocIndex.cc
#     analytics/Report.cc
#     analytics/ItemBoostExport.cc
#     analytics/ItemBoostMapper.cc
#     analytics/ItemBoostMerge.cc
#     analytics/TrafficSegment.cc
#     analytics/AnalyticsTableScan.cc
#     analytics/AnalyticsQuery.cc
#     analytics/AnalyticsQueryEngine.cc
#     analytics/AnalyticsQueryResult.cc
#     analytics/AnalyticsTableScanSource.cc
#     analytics/CSVSink.cc
#     cm-itemboostbuild.cc
#     ${PROTO_SRCS})
# 
# target_link_libraries(cm-itemboostbuild
#     ${CMAKE_THREAD_LIBS_INIT} ${FNORD_LIBS} ${FNORD_FTS_LIBS} ${FNORD_LIBS})

add_executable(cm-chunkserver
    $<TARGET_OBJECTS:cm-common>
    
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:stx-mdb>
    
    
    $<TARGET_OBJECTS:stx-rpc>
    
    
    
    $<TARGET_OBJECTS:tsdb-client>
    $<TARGET_OBJECTS:dproc>
    $<TARGET_OBJECTS:dproc>
    $<TARGET_OBJECTS:fnord-afx>
    ../3rdparty/libfnord/eventql/infra/sstable/SSTableServlet.cc
    ../3rdparty/libfnord/fnord-logtable/LogTableServlet.cc
    ../3rdparty/libfnord/fnord-logtable/TableReader.cc
    ../3rdparty/libfnord/fnord-logtable/TableWriter.cc
    ../3rdparty/libfnord/fnord-logtable/TableArena.cc
    ../3rdparty/libfnord/fnord-logtable/TableRepository.cc
    ../3rdparty/libfnord/fnord-logtable/TableJanitor.cc
    ../3rdparty/libfnord/fnord-logtable/TableReplication.cc
    ../3rdparty/libfnord/fnord-logtable/NumericBoundsSummary.cc
    ../3rdparty/libfnord/fnord-logtable/TableChunkSummaryReader.cc
    ../3rdparty/libfnord/fnord-logtable/TableChunkSummaryWriter.cc
    JoinedSessionViewer.cc
    ModelReplication.cc
    schemas.cc
    cm-chunkserver.cc)

target_link_libraries(cm-chunkserver
    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_LIBS})

add_executable(zbase
    $<TARGET_OBJECTS:cm-common>
    
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:stx-mdb>
    
    
    $<TARGET_OBJECTS:stx-rpc>
    
    
    
    $<TARGET_OBJECTS:tsdb-client>
    $<TARGET_OBJECTS:fnord-afx>
    $<TARGET_OBJECTS:dproc>
    ../3rdparty/libfnord/eventql/infra/sstable/SSTableServlet.cc
    ../3rdparty/libfnord/fnord-logtable/LogTableServlet.cc
    ../3rdparty/libfnord/fnord-logtable/TableReader.cc
    ../3rdparty/libfnord/fnord-logtable/TableWriter.cc
    ../3rdparty/libfnord/fnord-logtable/TableArena.cc
    ../3rdparty/libfnord/fnord-logtable/TableRepository.cc
    ../3rdparty/libfnord/fnord-logtable/TableJanitor.cc
    ../3rdparty/libfnord/fnord-logtable/TableReplication.cc
    ../3rdparty/libfnord/fnord-logtable/NumericBoundsSummary.cc
    ../3rdparty/libfnord/fnord-logtable/TableChunkSummaryReader.cc
    ../3rdparty/libfnord/fnord-logtable/TableChunkSummaryWriter.cc
    analytics/Report.cc
    analytics/AnalyticsTableScan.cc
    analytics/AnalyticsQuery.cc
    analytics/AnalyticsQueryEngine.cc
    analytics/AnalyticsQueryResult.cc
    analytics/CTRByPageQuery.cc
    analytics/CTRByPositionQuery.cc
    analytics/CTRByResultItemCategoryQuery.cc
    analytics/TopSearchTermsQuery.cc
    analytics/SearchCTRStats.cc
    analytics/DiscoveryDashboardQuery.cc
    analytics/CatalogDashboardQuery.cc
    analytics/CatalogCategoryDashboardQuery.cc
    analytics/CatalogCategoryDashboardResult.cc
    analytics/SearchDashboardQuery.cc
    analytics/ECommerceKPIQuery.cc
    analytics/TrafficSegment.cc
    analytics/CTRStatsServlet.cc
    analytics/AnalyticsServlet.cc
    analytics/ShopStatsTable.cc
    analytics/ShopStatsServlet.cc
    analytics/ShopStats.cc
    analytics/ECommerceRecoQueriesFeed.cc
    analytics/ECommerceSearchQueriesFeed.cc
    analytics/ECommercePreferenceSetsFeed.cc
    analytics/RecoEngineStatsQuery.cc
    analytics/RecoEngineStatsBreakdownQuery.cc
    analytics/JSONSink.cc
    analytics/FeedExportApp.cc
    analytics/AnalyticsApp.cc
    analytics/AnalyticsQueryMapper.cc
    analytics/AnalyticsQueryReducer.cc
    analytics/reports/ShopProductsTable.cc
    analytics/reports/ShopProductCTRStatsScan.cc
    analytics/ShopStatsApp.cc
    analytics/CTRByShopMapper.cc
    analytics/AnalyticsTableScanSource.cc
    analytics/ProductStatsByShopMapper.cc
    analytics/ECommerceStatsByShopMapper.cc
    analytics/RecoStats.cc
    analytics/ReportFactory.cc
    analytics/CSVExportRDD.cc
    analytics/reports/TopSearchTermsReport.cc
    analytics/reports/RecoEngineStatsReport.cc
    ../3rdparty/chartsql/src/chartsql/svalue.cc
    schemas.cc
    zbase.cc)

target_link_libraries(zbase
    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_LIBS} ${FNORD_FTS_LIBS})


# add_executable(cm-autocompleteserver
#     $<TARGET_OBJECTS:cm-common>
#     
#     $<TARGET_OBJECTS:fnord-feeds>
#     $<TARGET_OBJECTS:stx-mdb>
#     
#     
#     $<TARGET_OBJECTS:stx-rpc>
#     
#     $<TARGET_OBJECTS:nlputil>
#     3rdparty/libfnord/fnord-logtable/ArtifactIndex.cc
#     analytics/Report.cc
#     analytics/TermInfoTableSource.cc
#     analytics/TermInfo.cc
#     AutoCompleteServlet.cc
#     AutoCompleteModel.cc
#     ModelCache.cc
#     cm-autocompleteserver.cc)
# 
# target_link_libraries(cm-autocompleteserver
#     ${CMAKE_THREAD_LIBS_INIT} ${FNORD_LIBS} ${FNORD_FTS_LIBS})

#add_executable(cm-tokenize
#    $<TARGET_OBJECTS:cm-common>
#    
#    $<TARGET_OBJECTS:stx-mdb>
#    $<TARGET_OBJECTS:nlputil>
#    
#    
#    
#    cm-tokenize.cc)
#
#target_link_libraries(cm-tokenize
#    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_LIBS} ${FNORD_FTS_LIBS})
#
#add_executable(cm-jqcolumnize
#    $<TARGET_OBJECTS:cm-common>
#    
#    $<TARGET_OBJECTS:fnord-feeds>
#    $<TARGET_OBJECTS:stx-mdb>
#    
#    
#    $<TARGET_OBJECTS:stx-rpc>
#    
#    
#    
#    $<TARGET_OBJECTS:nlputil>
#    schemas.cc
#    analytics/AnalyticsTableScan.cc
#    analytics/TrafficSegment.cc
#    analytics/CTRByPositionQuery.cc
#    cm-jqcolumnize.cc)
#
#target_link_libraries(cm-jqcolumnize
#    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_LIBS} ${FNORD_FTS_LIBS})


#add_executable(tests/test-joinedsessionviewer
#    $<TARGET_OBJECTS:cm-common>
#    
#    
#    
#    
#    JoinedSessionViewer.cc
#    JoinedSession.pb.cc
#    JoinedSessionViewer_test.cc)
#
#target_link_libraries(tests/test-joinedsessionviewer ${CMAKE_THREAD_LIBS_INIT} ${FNORD_LIBS} ${FNORD_LIBS})
