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
    src/FeatureCache.cc
    src/FeatureID.cc
    src/FeaturePack.cc
    src/FeatureSchema.cc
    src/FeatureIndex.cc
    src/ItemRef.cc
    src/JoinedQuery.cc
    src/CustomerNamespace.cc
    src/PackedExample.cc)

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
    src/sellerstats/SellerStatsLookup.cc)

target_link_libraries(cm-frontend ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(cm-frontend m)

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
    src/index/IndexBuilder.cc)

target_link_libraries(cm-indexbuild
    ${CMAKE_THREAD_LIBS_INIT}
    ${FNORD_FTS_LIBS})

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
    src/cm-indexlookup.cc)

target_link_libraries(cm-indexlookup
    ${CMAKE_THREAD_LIBS_INIT}
    ${FNORD_FTS_LIBS})

add_executable(cm-sellerstatsbuild
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-sstable>
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:fnord-rpc>
    $<TARGET_OBJECTS:fnord-mdb>
    src/sellerstats/ActivityLog.cc
    src/cm-sellerstatsbuild.cc
    src/sellerstats/SellerStatsBuild.cc)

target_link_libraries(cm-sellerstatsbuild
    ${CMAKE_THREAD_LIBS_INIT})

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

add_executable(cm-featureselect
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-rpc>
    $<TARGET_OBJECTS:fnord-sstable>
    src/cm-featureselect.cc)

target_link_libraries(cm-featureselect
    ${CMAKE_THREAD_LIBS_INIT})

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
    src/cm-ctrstats-rollupflat.cc)

target_link_libraries(cm-ctrstats-rollup2d
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

target_link_libraries(cm-ctrstats-groupbyqueryfeature
    ${CMAKE_THREAD_LIBS_INIT})

add_executable(cm-reportserver
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-rpc>
    $<TARGET_OBJECTS:fnord-sstable>
    3rdparty/libfnord/fnord-sstable/SSTableServlet.cc
    src/cm-reportserver.cc)

target_link_libraries(cm-reportserver
    ${CMAKE_THREAD_LIBS_INIT})
