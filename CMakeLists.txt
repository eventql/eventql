cmake_minimum_required(VERSION 2.8.8)
include(FindPkgConfig)
include(CheckIncludeFile)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/modules/") 
project(cmapp)

option(ENABLE_TESTS "Build unit tests [default: ON]" ON)

include_directories(${PROJECT_BINARY_DIR})
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
include_directories(./)
include_directories(./src)
add_definitions(-ftemplate-depth=500)

if(APPLE)
  set(CMAKE_CXX_FLAGS "-std=c++0x -stdlib=libc++ ${CMAKE_CXX_FLAGS}")
else()
  set(CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")
endif()

find_package(Threads)

add_subdirectory(3rdparty/libfnord/fnord-base)
add_subdirectory(3rdparty/libfnord/fnord-feeds)
add_subdirectory(3rdparty/libfnord/fnord-fts)
add_subdirectory(3rdparty/libfnord/fnord-mdb)
add_subdirectory(3rdparty/libfnord/fnord-rpc)
add_subdirectory(3rdparty/libfnord/fnord-http)
add_subdirectory(3rdparty/libfnord/fnord-json)
add_subdirectory(3rdparty/libfnord/fnord-cstable)
add_subdirectory(3rdparty/libfnord/fnord-sstable)
add_subdirectory(3rdparty/libfnord/fnord-chart)
add_subdirectory(3rdparty/libfnord/fnord-metricdb)
add_subdirectory(3rdparty/libfnord/fnord-sql)
include_directories(3rdparty/libfnord/)
include_directories(3rdparty/libfnord/3rdparty/)
include_directories(${FNORD_FTS_INCLUDE})

add_library(cm-common OBJECT
    src/common.cc
    src/CTRCounter.cc
    src/Document.cc
    src/FeatureCache.cc
    src/FeatureID.cc
    src/FeaturePack.cc
    src/FeatureSchema.cc
    src/FeatureIndex.cc
    src/ItemRef.cc
    src/JoinedQuery.cc
    src/CustomerNamespace.cc
    src/PackedExample.cc)

# cm-frontend
set_source_files_properties(${CMAKE_BINARY_DIR}/cm-frontend-assets.cc PROPERTIES GENERATED true)

add_custom_target(cm-frontend-assets
    COMMAND ./3rdparty/libfnord/fnord-base/assets.sh ${CMAKE_BINARY_DIR}/cm-frontend-assets.cc < ./src/cm-frontend.assets
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(cm-frontend
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-sstable>
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:fnord-rpc>
    src/frontend/CMFrontend.cc
    src/frontend/LookupServlet.cc
    src/cm-frontend.cc
    src/sellerstats/ActivityLog.cc
    src/sellerstats/SellerStatsLookup.cc
    ${CMAKE_BINARY_DIR}/cm-frontend-assets.cc)

target_link_libraries(cm-frontend ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(cm-frontend m)
add_dependencies(cm-frontend cm-frontend-assets)

add_executable(cm-feedserver
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-sstable>
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:fnord-rpc>
    src/cm-feedserver.cc)

target_link_libraries(cm-feedserver ${CMAKE_THREAD_LIBS_INIT})

add_executable(cm-logjoin
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-sstable>
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-rpc>
    src/logjoin/TrackedQuery.cc
    src/logjoin/TrackedItemVisit.cc
    src/logjoin/TrackedSession.cc
    src/cm-logjoin.cc
    src/logjoin/LogJoin.cc
    src/logjoin/LogJoinShard.cc)

target_link_libraries(cm-logjoin ${CMAKE_THREAD_LIBS_INIT})

add_executable(cm-indexbuild
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-sstable>
    $<TARGET_OBJECTS:fnord-fts>
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:fnord-rpc>
    $<TARGET_OBJECTS:fnord-mdb>
    src/cm-indexbuild.cc
    src/IndexWriter.cc
    src/DocStore.cc
    src/FeatureIndexWriter.cc)

target_link_libraries(cm-indexbuild
    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_FTS_LIBS})

add_executable(cm-indexlookup
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-sstable>
    $<TARGET_OBJECTS:fnord-fts>
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:fnord-rpc>
    $<TARGET_OBJECTS:fnord-mdb>
    src/DocStore.cc
    src/IndexReader.cc
    src/FeatureIndexWriter.cc
    src/cm-indexlookup.cc)

target_link_libraries(cm-indexlookup
    ${CMAKE_THREAD_LIBS_INIT}
    ${FNORD_FTS_LIBS})

add_executable(cm-indexserver
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-sstable>
    $<TARGET_OBJECTS:fnord-fts>
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:fnord-rpc>
    $<TARGET_OBJECTS:fnord-mdb>
    src/DocStore.cc
    src/IndexReader.cc
    src/IndexServlet.cc
    src/SearchQuery.cc
    src/FeatureIndexWriter.cc
    src/cm-indexserver.cc)

target_link_libraries(cm-indexserver
    ${CMAKE_THREAD_LIBS_INIT}
    ${FNORD_FTS_LIBS})

#add_executable(cm-sellerstatsbuild
#    $<TARGET_OBJECTS:cm-common>
#    $<TARGET_OBJECTS:fnord-base>
#    $<TARGET_OBJECTS:fnord-http>
#    $<TARGET_OBJECTS:fnord-json>
#    $<TARGET_OBJECTS:fnord-sstable>
#    $<TARGET_OBJECTS:fnord-feeds>
#    $<TARGET_OBJECTS:fnord-rpc>
#    $<TARGET_OBJECTS:fnord-mdb>
#    src/sellerstats/ActivityLog.cc
#    src/cm-sellerstatsbuild.cc
#    src/sellerstats/SellerStatsBuild.cc)
#
#target_link_libraries(cm-sellerstatsbuild
#    ${CMAKE_THREAD_LIBS_INIT})

add_executable(cm-sellerstatslookup
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-sstable>
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:fnord-rpc>
    $<TARGET_OBJECTS:fnord-mdb>
    src/sellerstats/ActivityLog.cc
    src/cm-sellerstatslookup.cc
    src/sellerstats/SellerStatsLookup.cc)

target_link_libraries(cm-sellerstatslookup
    ${CMAKE_THREAD_LIBS_INIT}
    ${FNORD_FTS_LIBS})

add_executable(cm-dawandaindexpush
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-rpc>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-sql>
    $<TARGET_OBJECTS:fnord-sstable>
    customers/dawanda/cm-dawandaindexpush.cc)

target_link_libraries(cm-dawandaindexpush
    ${CMAKE_THREAD_LIBS_INIT}
    ${FNORD_SQL_LIBS})

add_executable(cm-featuredump
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-rpc>
    $<TARGET_OBJECTS:fnord-sstable>
    src/cm-featuredump.cc)

target_link_libraries(cm-featuredump
    ${CMAKE_THREAD_LIBS_INIT})

add_executable(cm-featureprep
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-rpc>
    $<TARGET_OBJECTS:fnord-sstable>
    $<TARGET_OBJECTS:fnord-fts>
    src/FeatureSelector.cc
    src/cm-featureprep.cc)

target_link_libraries(cm-featureprep
    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_FTS_LIBS})

add_executable(cm-ctrstats-rollupflat
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-rpc>
    $<TARGET_OBJECTS:fnord-sstable>
    src/cm-ctrstats-rollupflat.cc)

target_link_libraries(cm-ctrstats-rollupflat
    ${CMAKE_THREAD_LIBS_INIT})

add_executable(cm-ctrstats-rollup2d
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-rpc>
    $<TARGET_OBJECTS:fnord-sstable>
    src/cm-ctrstats-rollup2d.cc)

target_link_libraries(cm-ctrstats-rollup2d
    ${CMAKE_THREAD_LIBS_INIT})

add_executable(cm-ctrstats-rolluptimeseries
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-rpc>
    $<TARGET_OBJECTS:fnord-sstable>
    src/cm-ctrstats-rolluptimeseries.cc)

target_link_libraries(cm-ctrstats-rolluptimeseries
    ${CMAKE_THREAD_LIBS_INIT})

add_executable(cm-ctrstats-groupbyqueryfeature
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-rpc>
    $<TARGET_OBJECTS:fnord-sstable>
    src/cm-ctrstats-groupbyqueryfeature.cc)

target_link_libraries(cm-ctrstats-groupbyqueryfeature
    ${CMAKE_THREAD_LIBS_INIT})

add_executable(cm-ctrstats-groupbyqueryfeaturecrossitemfeature
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-rpc>
    $<TARGET_OBJECTS:fnord-sstable>
    src/cm-ctrstats-groupbyqueryfeaturecrossitemfeature.cc)

target_link_libraries(cm-ctrstats-groupbyqueryfeaturecrossitemfeature
    ${CMAKE_THREAD_LIBS_INIT})

add_executable(cm-ctrstats-groupbyqueryfeaturecrossitemterms
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-rpc>
    $<TARGET_OBJECTS:fnord-sstable>
    $<TARGET_OBJECTS:fnord-fts>
    src/cm-ctrstats-groupbyqueryfeaturecrossitemterms.cc)

target_link_libraries(cm-ctrstats-groupbyqueryfeaturecrossitemterms
    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_FTS_LIBS})

add_executable(cm-ctrstats-clickpagehisto
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-rpc>
    $<TARGET_OBJECTS:fnord-sstable>
    src/cm-ctrstats-clickpagehisto.cc)

target_link_libraries(cm-ctrstats-clickpagehisto
    ${CMAKE_THREAD_LIBS_INIT})

add_executable(cm-ctrstats-clickpositionhisto
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-rpc>
    $<TARGET_OBJECTS:fnord-sstable>
    src/cm-ctrstats-clickpositionhisto.cc)

target_link_libraries(cm-ctrstats-clickpositionhisto
    ${CMAKE_THREAD_LIBS_INIT})

add_executable(cm-ctrstats-builditemboost
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-rpc>
    $<TARGET_OBJECTS:fnord-sstable>
    $<TARGET_OBJECTS:fnord-fts>
    src/cm-ctrstats-builditemboost.cc)

target_link_libraries(cm-ctrstats-builditemboost
    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_FTS_LIBS})

add_executable(cm-reportbuild
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-rpc>
    $<TARGET_OBJECTS:fnord-sstable>
    $<TARGET_OBJECTS:fnord-fts>
    src/IndexReader.cc
    src/IndexWriter.cc
    src/FeatureIndexWriter.cc
    src/analytics/Report.cc
    src/analytics/ReportBuilder.cc
    src/analytics/JoinedQueryTableSource.cc
    src/analytics/CTRByPositionMapper.cc
    src/analytics/CTRByPageMapper.cc
    src/analytics/CTRStatsMapper.cc
    src/analytics/CTRCounterTableSink.cc
    src/analytics/CTRCounterTableSource.cc
    src/analytics/CTRCounterMergeReducer.cc
    #src/analytics/CTRBySearchQueryMapper.cc
    src/analytics/CTRBySearchTermCrossCategoryMapper.cc
    src/analytics/CTRByQueryAttributeMapper.cc
    src/analytics/RelatedTermsMapper.cc
    src/analytics/TermInfoTableSink.cc
    src/analytics/TermInfoTableSource.cc
    src/analytics/TermInfoMergeReducer.cc
    src/analytics/TermInfo.cc
    src/analytics/TopCategoriesByTermMapper.cc
    src/cm-reportbuild.cc)

target_link_libraries(cm-reportbuild
    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_FTS_LIBS})

add_executable(cm-reportserver
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-rpc>
    $<TARGET_OBJECTS:fnord-sstable>
    $<TARGET_OBJECTS:fnord-cstable>
    3rdparty/libfnord/fnord-sstable/SSTableServlet.cc
    src/analytics/AnalyticsTableScan.cc
    src/analytics/AnalyticsQuery.cc
    src/analytics/AnalyticsQueryEngine.cc
    src/analytics/AnalyticsQueryResult.cc
    src/analytics/CTRByPageQuery.cc
    src/analytics/CTRByPositionQuery.cc
    src/analytics/TopSearchQueriesQuery.cc
    src/analytics/DiscoveryKPIQuery.cc
    src/analytics/DiscoveryCategoryStatsQuery.cc
    src/analytics/DiscoveryCategoryStatsResult.cc
    src/analytics/TrafficSegment.cc
    src/analytics/CTRStatsServlet.cc
    src/analytics/AnalyticsServlet.cc
    src/analytics/CTRByPageServlet.cc
    src/cm-reportserver.cc)

target_link_libraries(cm-reportserver
    ${CMAKE_THREAD_LIBS_INIT})

add_executable(cm-autocompleteserver
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-rpc>
    $<TARGET_OBJECTS:fnord-sstable>
    $<TARGET_OBJECTS:fnord-fts>
    src/analytics/Report.cc
    src/analytics/TermInfoTableSource.cc
    src/analytics/TermInfo.cc
    src/AutoCompleteServlet.cc
    src/cm-autocompleteserver.cc)

target_link_libraries(cm-autocompleteserver
    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_FTS_LIBS})

add_executable(cm-tokenize
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-fts>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-sstable>
    src/cm-tokenize.cc)

target_link_libraries(cm-tokenize
    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_FTS_LIBS})

add_executable(cm-jqshuffle
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-rpc>
    $<TARGET_OBJECTS:fnord-sstable>
    src/cm-jqshuffle.cc)

target_link_libraries(cm-jqshuffle
    ${CMAKE_THREAD_LIBS_INIT})

add_executable(cm-jqcolumnize
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-rpc>
    $<TARGET_OBJECTS:fnord-cstable>
    $<TARGET_OBJECTS:fnord-sstable>
    $<TARGET_OBJECTS:fnord-fts>
    src/analytics/AnalyticsTableScan.cc
    src/analytics/TrafficSegment.cc
    src/analytics/CTRByPositionQuery.cc
    src/cm-jqcolumnize.cc)

target_link_libraries(cm-jqcolumnize
    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_FTS_LIBS})
