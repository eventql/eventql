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

set(CMAKE_CXX_FLAGS "-ftemplate-depth=500 ${CMAKE_CXX_FLAGS}")
# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -g")

if(APPLE)
  set(CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS} -stdlib=libc++ ")
else()
  set(CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "-ldl -static-libstdc++ -static-libgcc")
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
  set(BUILD_SHARED_LIBRARIES OFF)
  set(CMAKE_EXE_LINK_DYNAMIC_C_FLAGS)       # remove -Wl,-Bdynamic
  set(CMAKE_EXE_LINK_DYNAMIC_CXX_FLAGS)
  set(CMAKE_SHARED_LIBRARY_C_FLAGS)         # remove -fPIC
  set(CMAKE_SHARED_LIBRARY_CXX_FLAGS)
  set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)    # remove -rdynamic
  set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)
endif()

# strip binaries
#  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s")

find_package(Threads)

add_subdirectory(3rdparty/libfnord/fnord-base)
add_subdirectory(3rdparty/libfnord/fnord-msg)
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
add_subdirectory(3rdparty/libfnord/fnord-tsdb)
add_subdirectory(3rdparty/libfnord/fnord-dht)
add_subdirectory(3rdparty/libfnord/fnord-dproc)
add_subdirectory(3rdparty/libfnord/fnord-afx)
include_directories(3rdparty/libfnord/)
include_directories(3rdparty/libfnord/3rdparty/)
include_directories(${FNORD_FTS_INCLUDE})

file(GLOB PROTO_FILES src/*.proto)

foreach(PROTO_FILE ${PROTO_FILES})
  get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
  set(PROTO_SRCS ${PROTO_SRCS} ${CMAKE_CURRENT_SOURCE_DIR}/src/${PROTO_NAME}.pb.cc)
endforeach()

add_custom_target(cm-protos ALL
    COMMAND protoc --cpp_out=src/ -I${CMAKE_CURRENT_SOURCE_DIR}/src ${PROTO_FILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${PROTO_FILES})

set_source_files_properties(${PROTO_SRCS} PROPERTIES GENERATED true)

add_library(cm-common OBJECT
    src/common.cc
    src/CTRCounter.cc
    src/Document.cc
    src/ItemRef.cc
    src/CustomerNamespace.cc)

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
    $<TARGET_OBJECTS:fnord-msg>
    src/frontend/CMFrontend.cc
    src/frontend/IndexFeedUpload.cc
    src/cm-frontend.cc
    src/schemas.cc
    src/demoQueryServlet.cc
    ${CMAKE_BINARY_DIR}/cm-frontend-assets.cc)

target_link_libraries(cm-frontend ${CMAKE_THREAD_LIBS_INIT} ${FNORD_MSG_LIBS})
target_link_libraries(cm-frontend m)
add_dependencies(cm-frontend cm-frontend-assets)

#add_executable(cm-feedserver
#    $<TARGET_OBJECTS:cm-common>
#    $<TARGET_OBJECTS:fnord-base>
#    $<TARGET_OBJECTS:fnord-http>
#    $<TARGET_OBJECTS:fnord-mdb>
#    $<TARGET_OBJECTS:fnord-json>
#    $<TARGET_OBJECTS:fnord-sstable>
#    $<TARGET_OBJECTS:fnord-feeds>
#    $<TARGET_OBJECTS:fnord-rpc>
#    src/cm-feedserver.cc)
#
#target_link_libraries(cm-feedserver ${CMAKE_THREAD_LIBS_INIT} ${FNORD_MSG_LIBS})

add_executable(cm-logjoin
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-sstable>
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-rpc>
    $<TARGET_OBJECTS:fnord-msg>
    $<TARGET_OBJECTS:fnord-fts>
    src/logjoin/TrackedCartItem.cc
    src/logjoin/TrackedItemVisit.cc
    src/logjoin/TrackedQuery.cc
    src/logjoin/TrackedSession.cc
    src/DocIndex.cc
    src/cm-logjoin.cc
    src/schemas.cc
    src/logjoin/LogJoin.cc
    src/logjoin/LogJoinUpload.cc
    src/logjoin/LogJoinExport.cc
    src/logjoin/LogJoinTarget.cc
    src/logjoin/LogJoinShard.cc
    ${PROTO_SRCS})

target_link_libraries(cm-logjoin ${CMAKE_THREAD_LIBS_INIT} ${FNORD_MSG_LIBS} ${FNORD_FTS_LIBS})

add_executable(tests/test-logjoin
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-sstable>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-rpc>
    $<TARGET_OBJECTS:fnord-msg>
    $<TARGET_OBJECTS:fnord-fts>
    src/logjoin/TrackedCartItem.cc
    src/logjoin/TrackedItemVisit.cc
    src/logjoin/TrackedQuery.cc
    src/logjoin/TrackedSession.cc
    src/JoinedSession.pb.cc
    src/LogJoin_test.cc)

target_link_libraries(tests/test-logjoin ${CMAKE_THREAD_LIBS_INIT} ${FNORD_MSG_LIBS} ${FNORD_FTS_LIBS})

#add_executable(cm-logjoin-backfill
#    $<TARGET_OBJECTS:cm-common>
#    $<TARGET_OBJECTS:fnord-base>
#    $<TARGET_OBJECTS:fnord-feeds>
#    $<TARGET_OBJECTS:fnord-mdb>
#    $<TARGET_OBJECTS:fnord-http>
#    $<TARGET_OBJECTS:fnord-json>
#    $<TARGET_OBJECTS:fnord-rpc>
#    $<TARGET_OBJECTS:fnord-cstable>
#    $<TARGET_OBJECTS:fnord-sstable>
#    $<TARGET_OBJECTS:fnord-fts>
#    $<TARGET_OBJECTS:fnord-msg>
#    3rdparty/libfnord/fnord-logtable/TableReader.cc
#    3rdparty/libfnord/fnord-logtable/TableArena.cc
#    3rdparty/libfnord/fnord-logtable/TableWriter.cc
#    3rdparty/libfnord/fnord-logtable/TableRepository.cc
#    3rdparty/libfnord/fnord-logtable/ArtifactIndex.cc
#    3rdparty/libfnord/fnord-logtable/NumericBoundsSummary.cc
#    3rdparty/libfnord/fnord-logtable/TableChunkSummaryReader.cc
#    3rdparty/libfnord/fnord-logtable/TableChunkSummaryWriter.cc
#    3rdparty/libfnord/fnord-logtable/LogTableTail.cc
#    src/logjoin/LogJoinBackfill.cc
#    src/schemas.cc
#    src/DocIndex.cc
#    src/cm-logjoin-backfill.cc)
#
#target_link_libraries(cm-logjoin-backfill
#    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_MSG_LIBS} ${FNORD_FTS_LIBS})
#
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
    $<TARGET_OBJECTS:fnord-msg>
    $<TARGET_OBJECTS:fnord-cstable>
    $<TARGET_OBJECTS:fnord-afx>
    src/cm-indexbuild.cc
    3rdparty/libfnord/fnord-logtable/RemoteTableReader.cc
    3rdparty/libfnord/fnord-logtable/TableReader.cc
    3rdparty/libfnord/fnord-logtable/TableArena.cc
    3rdparty/libfnord/fnord-logtable/TableWriter.cc
    3rdparty/libfnord/fnord-logtable/TableRepository.cc
    3rdparty/libfnord/fnord-logtable/NumericBoundsSummary.cc
    3rdparty/libfnord/fnord-logtable/TableChunkSummaryReader.cc
    3rdparty/libfnord/fnord-logtable/TableChunkSummaryWriter.cc
    3rdparty/libfnord/fnord-logtable/LogTableTail.cc
    src/schemas.cc
    src/DocStore.cc
    src/DocIndex.cc)

target_link_libraries(cm-indexbuild
    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_MSG_LIBS} ${FNORD_FTS_LIBS})

#add_executable(cm-indexlookup
#    $<TARGET_OBJECTS:cm-common>
#    $<TARGET_OBJECTS:fnord-base>
#    $<TARGET_OBJECTS:fnord-http>
#    $<TARGET_OBJECTS:fnord-json>
#    $<TARGET_OBJECTS:fnord-sstable>
#    $<TARGET_OBJECTS:fnord-fts>
#    $<TARGET_OBJECTS:fnord-feeds>
#    $<TARGET_OBJECTS:fnord-rpc>
#    $<TARGET_OBJECTS:fnord-mdb>
#    src/DocStore.cc
#    src/DocIndex.cc
#    src/cm-indexlookup.cc)
#
#target_link_libraries(cm-indexlookup
#    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_MSG_LIBS}
#    ${FNORD_FTS_LIBS})
#
#add_executable(cm-indexserver
#    $<TARGET_OBJECTS:cm-common>
#    $<TARGET_OBJECTS:fnord-base>
#    $<TARGET_OBJECTS:fnord-http>
#    $<TARGET_OBJECTS:fnord-json>
#    $<TARGET_OBJECTS:fnord-sstable>
#    $<TARGET_OBJECTS:fnord-fts>
#    $<TARGET_OBJECTS:fnord-feeds>
#    $<TARGET_OBJECTS:fnord-rpc>
#    $<TARGET_OBJECTS:fnord-mdb>
#    src/DocStore.cc
#    src/IndexServlet.cc
#    src/SearchQuery.cc
#    src/DocIndex.cc
#    src/cm-indexserver.cc)
#
#target_link_libraries(cm-indexserver
#    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_MSG_LIBS}
#    ${FNORD_FTS_LIBS})

#add_executable(cm-dawandaindexpush
#    $<TARGET_OBJECTS:cm-common>
#    $<TARGET_OBJECTS:fnord-base>
#    $<TARGET_OBJECTS:fnord-http>
#    $<TARGET_OBJECTS:fnord-json>
#    $<TARGET_OBJECTS:fnord-rpc>
#    $<TARGET_OBJECTS:fnord-mdb>
#    $<TARGET_OBJECTS:fnord-sql>
#    $<TARGET_OBJECTS:fnord-sstable>
#    customers/dawanda/cm-dawandaindexpush.cc)
#
#target_link_libraries(cm-dawandaindexpush
#    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_MSG_LIBS})

#add_executable(cm-featuredump
#    $<TARGET_OBJECTS:cm-common>
#    $<TARGET_OBJECTS:fnord-base>
#    $<TARGET_OBJECTS:fnord-feeds>
#    $<TARGET_OBJECTS:fnord-http>
#    $<TARGET_OBJECTS:fnord-mdb>
#    $<TARGET_OBJECTS:fnord-json>
#    $<TARGET_OBJECTS:fnord-rpc>
#    $<TARGET_OBJECTS:fnord-sstable>
#    src/cm-featuredump.cc)
#
#target_link_libraries(cm-featuredump
#    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_MSG_LIBS})
#
#add_executable(cm-featureprep
#    $<TARGET_OBJECTS:cm-common>
#    $<TARGET_OBJECTS:fnord-base>
#    $<TARGET_OBJECTS:fnord-feeds>
#    $<TARGET_OBJECTS:fnord-mdb>
#    $<TARGET_OBJECTS:fnord-http>
#    $<TARGET_OBJECTS:fnord-json>
#    $<TARGET_OBJECTS:fnord-rpc>
#    $<TARGET_OBJECTS:fnord-sstable>
#    $<TARGET_OBJECTS:fnord-fts>
#    src/FeatureSelector.cc
#    src/cm-featureprep.cc)
#
#target_link_libraries(cm-featureprep
#    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_MSG_LIBS} ${FNORD_FTS_LIBS})

#add_executable(cm-ctrstats-builditemboost
#    $<TARGET_OBJECTS:cm-common>
#    $<TARGET_OBJECTS:fnord-base>
#    $<TARGET_OBJECTS:fnord-feeds>
#    $<TARGET_OBJECTS:fnord-mdb>
#    $<TARGET_OBJECTS:fnord-http>
#    $<TARGET_OBJECTS:fnord-json>
#    $<TARGET_OBJECTS:fnord-rpc>
#    $<TARGET_OBJECTS:fnord-sstable>
#    $<TARGET_OBJECTS:fnord-fts>
#    src/cm-ctrstats-builditemboost.cc)
#
#target_link_libraries(cm-ctrstats-builditemboost
#    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_MSG_LIBS} ${FNORD_FTS_LIBS})

#add_executable(cm-reportbuild
#    $<TARGET_OBJECTS:cm-common>
#    $<TARGET_OBJECTS:fnord-base>
#    $<TARGET_OBJECTS:fnord-feeds>
#    $<TARGET_OBJECTS:fnord-mdb>
#    $<TARGET_OBJECTS:fnord-http>
#    $<TARGET_OBJECTS:fnord-json>
#    $<TARGET_OBJECTS:fnord-rpc>
#    $<TARGET_OBJECTS:fnord-sstable>
#    $<TARGET_OBJECTS:fnord-fts>
#    src/IndexReader.cc
#    src/IndexWriter.cc
#    src/DocIndex.cc
#    src/analytics/Report.cc
#    src/analytics/ReportBuilder.cc
#    src/analytics/CTRByPageMapper.cc
#    src/analytics/CTRStatsMapper.cc
#    src/analytics/CTRCounterTableSink.cc
#    src/analytics/CTRCounterTableSource.cc
#    src/analytics/CTRCounterMergeReducer.cc
#    src/analytics/CTRBySearchTermCrossCategoryMapper.cc
#    src/analytics/CTRByQueryAttributeMapper.cc
#    src/analytics/RelatedTermsMapper.cc
#    src/analytics/TermInfoTableSink.cc
#    src/analytics/TermInfoTableSource.cc
#    src/analytics/TermInfoMergeReducer.cc
#    src/analytics/TermInfo.cc
#    src/analytics/TopCategoriesByTermMapper.cc
#    src/cm-reportbuild.cc)
#
#target_link_libraries(cm-reportbuild
#    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_MSG_LIBS} ${FNORD_FTS_LIBS})

#add_executable(cm-autocompletebuild
#    $<TARGET_OBJECTS:cm-common>
#    $<TARGET_OBJECTS:fnord-base>
#    $<TARGET_OBJECTS:fnord-feeds>
#    $<TARGET_OBJECTS:fnord-mdb>
#    $<TARGET_OBJECTS:fnord-http>
#    $<TARGET_OBJECTS:fnord-json>
#    $<TARGET_OBJECTS:fnord-rpc>
#    $<TARGET_OBJECTS:fnord-cstable>
#    $<TARGET_OBJECTS:fnord-sstable>
#    $<TARGET_OBJECTS:fnord-fts>
#    $<TARGET_OBJECTS:fnord-msg>
#    3rdparty/libfnord/fnord-logtable/TableReader.cc
#    3rdparty/libfnord/fnord-logtable/TableArena.cc
#    3rdparty/libfnord/fnord-logtable/TableWriter.cc
#    3rdparty/libfnord/fnord-logtable/TableRepository.cc
#    3rdparty/libfnord/fnord-logtable/ArtifactIndex.cc
#    3rdparty/libfnord/fnord-logtable/NumericBoundsSummary.cc
#    3rdparty/libfnord/fnord-logtable/TableChunkSummaryReader.cc
#    3rdparty/libfnord/fnord-logtable/TableChunkSummaryWriter.cc
#    src/schemas.cc
#    src/DocIndex.cc
#    src/analytics/Report.cc
#    src/analytics/ReportBuilder.cc
#    src/analytics/CTRCounterTableSink.cc
#    src/analytics/CTRCounterTableSource.cc
#    src/analytics/CTRCounterMergeReducer.cc
#    src/analytics/TrafficSegment.cc
#    src/analytics/AnalyticsTableScan.cc
#    src/analytics/AnalyticsQuery.cc
#    src/analytics/AnalyticsQueryEngine.cc
#    src/analytics/AnalyticsQueryResult.cc
#    src/analytics/CTRBySearchTermCrossCategoryMapper.cc
#    src/analytics/AnalyticsTableScanSource.cc
#    src/analytics/RelatedTermsMapper.cc
#    src/analytics/TermInfoTableSink.cc
#    src/analytics/TermInfoTableSource.cc
#    src/analytics/TermInfoMergeReducer.cc
#    src/analytics/TermInfo.cc
#    src/analytics/TopCategoriesByTermMapper.cc
#    src/cm-autocompletebuild.cc)
#
#target_link_libraries(cm-autocompletebuild
#    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_MSG_LIBS} ${FNORD_FTS_LIBS})

add_executable(cm-shopstatsbuild
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
    $<TARGET_OBJECTS:fnord-msg>
    $<TARGET_OBJECTS:fnord-tsdb>
    $<TARGET_OBJECTS:fnord-dproc>
    $<TARGET_OBJECTS:fnord-afx>
    3rdparty/libfnord/fnord-logtable/TableReader.cc
    3rdparty/libfnord/fnord-logtable/TableArena.cc
    3rdparty/libfnord/fnord-logtable/TableWriter.cc
    3rdparty/libfnord/fnord-logtable/TableRepository.cc
    3rdparty/libfnord/fnord-logtable/NumericBoundsSummary.cc
    3rdparty/libfnord/fnord-logtable/TableChunkSummaryReader.cc
    3rdparty/libfnord/fnord-logtable/TableChunkSummaryWriter.cc
    src/schemas.cc
    src/DocIndex.cc
    src/analytics/Report.cc
    src/analytics/CTRByShopMapper.cc
    src/analytics/ECommerceStatsByShopMapper.cc
    src/analytics/ProductStatsByShopMapper.cc
    src/analytics/TrafficSegment.cc
    src/analytics/AnalyticsTableScan.cc
    src/analytics/AnalyticsQuery.cc
    src/analytics/AnalyticsQueryEngine.cc
    src/analytics/AnalyticsQueryResult.cc
    src/analytics/AnalyticsTableScanSource.cc
    src/analytics/ShopStats.cc
    src/analytics/CTRCounterTableSource.cc
    src/analytics/CTRCounterTableSink.cc
    src/analytics/CTRCounterMergeReducer.cc
    src/analytics/CTRBySearchTermCrossCategoryMapper.cc
    src/analytics/TopTermsByCategoryMapper.cc
    src/cm-shopstatsbuild.cc
    ${PROTO_SRCS})

target_link_libraries(cm-shopstatsbuild
    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_MSG_LIBS} ${FNORD_FTS_LIBS})

add_executable(cm-itemboostbuild
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
    $<TARGET_OBJECTS:fnord-msg>
    $<TARGET_OBJECTS:fnord-tsdb>
    $<TARGET_OBJECTS:fnord-dproc>
    src/schemas.cc
    src/DocIndex.cc
    src/analytics/Report.cc
    src/analytics/ItemBoostExport.cc
    src/analytics/ItemBoostMapper.cc
    src/analytics/ItemBoostMerge.cc
    src/analytics/TrafficSegment.cc
    src/analytics/AnalyticsTableScan.cc
    src/analytics/AnalyticsQuery.cc
    src/analytics/AnalyticsQueryEngine.cc
    src/analytics/AnalyticsQueryResult.cc
    src/analytics/AnalyticsTableScanSource.cc
    src/analytics/CSVSink.cc
    src/cm-itemboostbuild.cc
    ${PROTO_SRCS})

target_link_libraries(cm-itemboostbuild
    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_MSG_LIBS} ${FNORD_FTS_LIBS} ${FNORD_MSG_LIBS})

add_executable(cm-feedexport
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
    $<TARGET_OBJECTS:fnord-msg>
    $<TARGET_OBJECTS:fnord-tsdb>
    $<TARGET_OBJECTS:fnord-dproc>
    src/schemas.cc
    src/DocIndex.cc
    src/analytics/Report.cc
    src/analytics/ECommerceRecoQueriesFeed.cc
    src/analytics/ECommerceSearchQueriesFeed.cc
    src/analytics/TrafficSegment.cc
    src/analytics/JSONSink.cc
    src/analytics/AnalyticsTableScan.cc
    src/analytics/AnalyticsQuery.cc
    src/analytics/AnalyticsQueryEngine.cc
    src/analytics/AnalyticsQueryResult.cc
    src/analytics/AnalyticsTableScanSource.cc
    src/cm-feedexport.cc
    ${PROTO_SRCS})

target_link_libraries(cm-feedexport
    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_MSG_LIBS} ${FNORD_FTS_LIBS} ${FNORD_MSG_LIBS})


add_executable(cm-chunkserver
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-rpc>
    $<TARGET_OBJECTS:fnord-sstable>
    $<TARGET_OBJECTS:fnord-cstable>
    $<TARGET_OBJECTS:fnord-msg>
    $<TARGET_OBJECTS:fnord-tsdb>
    $<TARGET_OBJECTS:fnord-dht>
    $<TARGET_OBJECTS:fnord-afx>
    3rdparty/libfnord/fnord-sstable/SSTableServlet.cc
    3rdparty/libfnord/fnord-logtable/LogTableServlet.cc
    3rdparty/libfnord/fnord-logtable/TableReader.cc
    3rdparty/libfnord/fnord-logtable/TableWriter.cc
    3rdparty/libfnord/fnord-logtable/TableArena.cc
    3rdparty/libfnord/fnord-logtable/TableRepository.cc
    3rdparty/libfnord/fnord-logtable/TableJanitor.cc
    3rdparty/libfnord/fnord-logtable/TableReplication.cc
    3rdparty/libfnord/fnord-logtable/NumericBoundsSummary.cc
    3rdparty/libfnord/fnord-logtable/TableChunkSummaryReader.cc
    3rdparty/libfnord/fnord-logtable/TableChunkSummaryWriter.cc
    src/JoinedSessionViewer.cc
    src/ModelReplication.cc
    src/schemas.cc
    src/cm-chunkserver.cc
    ${PROTO_SRCS})

target_link_libraries(cm-chunkserver
    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_MSG_LIBS})

add_executable(cm-analytics-backend
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-rpc>
    $<TARGET_OBJECTS:fnord-sstable>
    $<TARGET_OBJECTS:fnord-cstable>
    $<TARGET_OBJECTS:fnord-msg>
    $<TARGET_OBJECTS:fnord-tsdb>
    $<TARGET_OBJECTS:fnord-afx>
    3rdparty/libfnord/fnord-sstable/SSTableServlet.cc
    3rdparty/libfnord/fnord-logtable/LogTableServlet.cc
    3rdparty/libfnord/fnord-logtable/TableReader.cc
    3rdparty/libfnord/fnord-logtable/TableWriter.cc
    3rdparty/libfnord/fnord-logtable/TableArena.cc
    3rdparty/libfnord/fnord-logtable/TableRepository.cc
    3rdparty/libfnord/fnord-logtable/TableJanitor.cc
    3rdparty/libfnord/fnord-logtable/TableReplication.cc
    3rdparty/libfnord/fnord-logtable/NumericBoundsSummary.cc
    3rdparty/libfnord/fnord-logtable/TableChunkSummaryReader.cc
    3rdparty/libfnord/fnord-logtable/TableChunkSummaryWriter.cc
    src/analytics/AnalyticsTableScan.cc
    src/analytics/AnalyticsQuery.cc
    src/analytics/AnalyticsQueryEngine.cc
    src/analytics/AnalyticsQueryResult.cc
    src/analytics/CTRByPageQuery.cc
    src/analytics/CTRByPositionQuery.cc
    src/analytics/CTRByResultItemCategoryQuery.cc
    src/analytics/TopSearchQueriesQuery.cc
    src/analytics/DiscoveryStats.cc
    src/analytics/DiscoveryDashboardQuery.cc
    src/analytics/DiscoveryCatalogStatsQuery.cc
    src/analytics/DiscoveryCategoryStatsQuery.cc
    src/analytics/DiscoveryCategoryStatsResult.cc
    src/analytics/DiscoverySearchStatsQuery.cc
    src/analytics/ECommerceKPIQuery.cc
    src/analytics/TrafficSegment.cc
    src/analytics/CTRStatsServlet.cc
    src/analytics/AnalyticsServlet.cc
    src/analytics/ShopStatsTable.cc
    src/analytics/ShopStatsServlet.cc
    src/analytics/ShopStats.cc
    src/demoQueryServlet.cc
    src/schemas.cc
    src/cm-analytics-backend.cc)

target_link_libraries(cm-analytics-backend
    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_MSG_LIBS} ${FNORD_FTS_LIBS})


# add_executable(cm-autocompleteserver
#     $<TARGET_OBJECTS:cm-common>
#     $<TARGET_OBJECTS:fnord-base>
#     $<TARGET_OBJECTS:fnord-feeds>
#     $<TARGET_OBJECTS:fnord-mdb>
#     $<TARGET_OBJECTS:fnord-http>
#     $<TARGET_OBJECTS:fnord-json>
#     $<TARGET_OBJECTS:fnord-rpc>
#     $<TARGET_OBJECTS:fnord-sstable>
#     $<TARGET_OBJECTS:fnord-fts>
#     3rdparty/libfnord/fnord-logtable/ArtifactIndex.cc
#     src/analytics/Report.cc
#     src/analytics/TermInfoTableSource.cc
#     src/analytics/TermInfo.cc
#     src/AutoCompleteServlet.cc
#     src/AutoCompleteModel.cc
#     src/ModelCache.cc
#     src/cm-autocompleteserver.cc)
# 
# target_link_libraries(cm-autocompleteserver
#     ${CMAKE_THREAD_LIBS_INIT} ${FNORD_MSG_LIBS} ${FNORD_FTS_LIBS})

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
    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_MSG_LIBS} ${FNORD_FTS_LIBS})

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
    $<TARGET_OBJECTS:fnord-msg>
    $<TARGET_OBJECTS:fnord-fts>
    src/schemas.cc
    src/analytics/AnalyticsTableScan.cc
    src/analytics/TrafficSegment.cc
    src/analytics/CTRByPositionQuery.cc
    src/cm-jqcolumnize.cc)

target_link_libraries(cm-jqcolumnize
    ${CMAKE_THREAD_LIBS_INIT} ${FNORD_MSG_LIBS} ${FNORD_FTS_LIBS})


#add_executable(tests/test-joinedsessionviewer
#    $<TARGET_OBJECTS:cm-common>
#    $<TARGET_OBJECTS:fnord-base>
#    $<TARGET_OBJECTS:fnord-json>
#    $<TARGET_OBJECTS:fnord-sstable>
#    $<TARGET_OBJECTS:fnord-http>
#    src/JoinedSessionViewer.cc
#    src/JoinedSession.pb.cc
#    src/JoinedSessionViewer_test.cc)
#
#target_link_libraries(tests/test-joinedsessionviewer ${CMAKE_THREAD_LIBS_INIT} ${FNORD_MSG_LIBS} ${FNORD_MSG_LIBS})
