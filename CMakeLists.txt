cmake_minimum_required(VERSION 2.8.8)
include(FindPkgConfig)
include(CheckIncludeFile)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/modules/") 
project(cmapp)

option(ENABLE_TESTS "Build unit tests [default: ON]" ON)

include_directories(${PROJECT_BINARY_DIR})
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
include_directories(./)

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
    cm-common/common.cc
    cm-common/FeatureID.cc
    cm-common/FeaturePack.cc
    cm-common/FeatureSchema.cc
    cm-common/FeatureIndex.cc
    cm-common/ItemRef.cc
    cm-common/JoinedQuery.cc
    cm-common/CustomerNamespace.cc
    cm-common/PackedExample.cc)

add_executable(cm-frontend
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:fnord-sstable>
    $<TARGET_OBJECTS:fnord-feeds>
    $<TARGET_OBJECTS:fnord-rpc>
    cm-frontend/CMFrontend.cc
    cm-frontend/cm-frontend.cc
    cm-frontend/LookupServlet.cc
    cm-sellerstats/ActivityLog.cc
    cm-sellerstats/SellerStatsLookup.cc)

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
    cm-common/cm-feedserver.cc)

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
    cm-logjoin/TrackedQuery.cc
    cm-logjoin/TrackedItemVisit.cc
    cm-logjoin/TrackedSession.cc
    cm-logjoin/cm-logjoin.cc
    cm-logjoin/LogJoin.cc
    cm-logjoin/LogJoinShard.cc)

target_link_libraries(cm-logjoin ${CMAKE_THREAD_LIBS_INIT})

# add_executable(cm-crawlworker
#     $<TARGET_OBJECTS:cm-common>
#     $<TARGET_OBJECTS:fnord-base>
#     $<TARGET_OBJECTS:fnord-http>
#     $<TARGET_OBJECTS:fnord-json>
#     $<TARGET_OBJECTS:fnord-redis>
#     $<TARGET_OBJECTS:fnord-sstable>
#     $<TARGET_OBJECTS:fnord-logstream-service>
#     cm-common/crawler/crawler.cc
#     cm-common/cm-crawlworker.cc)
# 
# target_link_libraries(cm-crawlworker ${CMAKE_THREAD_LIBS_INIT})
# 
add_executable(cm-stats
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-chart>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-sstable>
    $<TARGET_OBJECTS:fnord-metricdb>
    $<TARGET_OBJECTS:fnord-rpc>
    cm-stats/cm-stats.cc)

target_link_libraries(cm-stats ${CMAKE_THREAD_LIBS_INIT})

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
    cm-index/cm-indexbuild.cc
    cm-index/IndexBuilder.cc)

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
    cm-index/cm-indexlookup.cc)

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
    cm-sellerstats/ActivityLog.cc
    cm-sellerstats/cm-sellerstatsbuild.cc
    cm-sellerstats/SellerStatsBuild.cc)

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
    cm-sellerstats/ActivityLog.cc
    cm-sellerstats/cm-sellerstatslookup.cc
    cm-sellerstats/SellerStatsLookup.cc)

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
    cm-dawandaconnect/cm-dawandaindexpush.cc)

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
    cm-logreg/cm-featuredump.cc)

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
    cm-logreg/cm-featureselect.cc)

target_link_libraries(cm-featureselect
    ${CMAKE_THREAD_LIBS_INIT})
