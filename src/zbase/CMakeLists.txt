include_directories(${CMAKE_CURRENT_BINARY_DIR}/)

file(GLOB PROTO_FILES *.proto)
STX_PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

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

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/buildconfig.h.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/buildconfig.h)

file(GLOB COREPROTO_FILES core/*.proto)
STX_PROTOBUF_GENERATE_CPP(COREPROTO_SRCS COREPROTO_HDRS ${COREPROTO_FILES})

add_library(tsdb-client OBJECT
    core/TSDBClient.cc
    core/TimeWindowPartitioner.cc
    core/FixedShardPartitioner.cc
    ${COREPROTO_SRCS})

add_library(coreprotos OBJECT
    ${COREPROTO_SRCS})

add_library(customerdirectory OBJECT
    CustomerConfig.cc
    ConfigDirectory.cc
    SessionSchema.cc
    TableDefinition.cc
    AnalyticsAuth.cc
    ${PROTO_SRCS}
    ${ZBASE_ASSETS_SRCS})

add_dependencies(customerdirectory coreprotos)

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
    core/LogPartitionReader.cc
    core/LogPartitionWriter.cc
    core/LogPartitionReplication.cc
    core/LazyPartition.cc
    core/Table.cc
    core/ReplicationWorker.cc
    core/ReplicationScheme.cc
    core/ReplicationState.cc
    core/RemoteTSDBScan.cc
    core/StaticPartitionReader.cc
    core/StaticPartitionWriter.cc
    core/StaticPartitionReplication.cc)

add_dependencies(tsdb-server customerdirectory)
add_dependencies(tsdb-server coreprotos)

add_executable(test-recordset
    $<TARGET_OBJECTS:stx-mdb>
    $<TARGET_OBJECTS:tsdb-server>
    $<TARGET_OBJECTS:coreprotos>
    $<TARGET_OBJECTS:dproc>
    $<TARGET_OBJECTS:customerdirectory>
    core/RecordSet_test.cc)

target_link_libraries(test-recordset chartsql cplot sstable cstable stx-http stx-protobuf stx-json stx-base pcre)

add_executable(test-tsdb-timewindowpartitioner
    $<TARGET_OBJECTS:stx-mdb>
    $<TARGET_OBJECTS:tsdb-server>
    $<TARGET_OBJECTS:coreprotos>
    $<TARGET_OBJECTS:dproc>
    $<TARGET_OBJECTS:customerdirectory>
    core/TimeWindowPartitioner_test.cc)

target_link_libraries(test-tsdb-timewindowpartitioner chartsql cplot sstable cstable stx-http stx-protobuf stx-json stx-base pcre)

add_executable(test-replicationscheme
    $<TARGET_OBJECTS:stx-mdb>
    $<TARGET_OBJECTS:tsdb-server>
    $<TARGET_OBJECTS:coreprotos>
    $<TARGET_OBJECTS:dproc>
    $<TARGET_OBJECTS:customerdirectory>
    core/ReplicationScheme_test.cc)

target_link_libraries(test-replicationscheme chartsql cplot sstable cstable stx-http stx-protobuf stx-json stx-base pcre)

add_subdirectory(util)
add_subdirectory(util/mysql)
add_subdirectory(util/mdb)

add_custom_command(
    OUTPUT mapreduce_preludejs.cc
    COMMAND 
        echo 'zbase/mapreduce/prelude.js:${CMAKE_CURRENT_SOURCE_DIR}/mapreduce/prelude.js' | ${CMAKE_SOURCE_DIR}/deps/libstx/src/stx/assets.sh mapreduce_preludejs.cc
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/mapreduce/prelude.js")

add_executable(zbase
    $<TARGET_OBJECTS:stx-mdb>
    $<TARGET_OBJECTS:brokerd-client>
    $<TARGET_OBJECTS:dxa-docdb>
    $<TARGET_OBJECTS:tsdb-server>
    $<TARGET_OBJECTS:coreprotos>
    $<TARGET_OBJECTS:customerdirectory>
    #$<TARGET_OBJECTS:fnord-afx>
    $<TARGET_OBJECTS:dproc>
    #sstable/SSTableServlet.cc
    api/LogfileService.cc
    api/LogfileAPIServlet.cc
    api/EventsService.cc
    api/EventsAPIServlet.cc
    api/MapReduceService.cc
    api/MapReduceAPIServlet.cc
    mapreduce/MapReduceTask.cc
    mapreduce/MapReduceTaskBuilder.cc
    mapreduce/MapReduceScheduler.cc
    mapreduce/tasks/MapTableTask.cc
    mapreduce/tasks/ReduceTask.cc
    mapreduce/tasks/ReturnResultsTask.cc
    mapreduce/tasks/SaveToTableTask.cc
    mapreduce/tasks/SaveToTablePartitionTask.cc
    LogfileScanResult.cc
    EventScanResult.cc
    Report.cc
    CTRStatsServlet.cc
    AnalyticsServlet.cc
    HTTPAuth.cc
    JSONSink.cc
    AnalyticsApp.cc
    DrilldownTree.cc
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
    JavaScriptContext.cc
    WebUIServlet.cc
    WebDocsServlet.cc
    ${CMAKE_CURRENT_BINARY_DIR}/mapreduce_preludejs.cc
    zbase.cc)

target_link_libraries(zbase chartsql cplot sstable cstable stx-http stx-json stx-protobuf stx-base stx-rpc js_static.a z pcre)
target_link_libraries(zbase -L${CMAKE_BINARY_DIR}/deps/3rdparty/mozjs/dist/lib -ljs_static)
add_dependencies(zbase customerdirectory mozjs_build)

add_executable(zbasectl
    $<TARGET_OBJECTS:stx-mdb>
    $<TARGET_OBJECTS:brokerd-client>
    $<TARGET_OBJECTS:tsdb-server>
    $<TARGET_OBJECTS:coreprotos>
    $<TARGET_OBJECTS:customerdirectory>
    #$<TARGET_OBJECTS:fnord-afx>
    $<TARGET_OBJECTS:dproc>
    #sstable/SSTableServlet.cc
    zbasectl.cc)

target_link_libraries(zbasectl chartsql cplot sstable cstable stx-http stx-json stx-protobuf stx-base stx-rpc pcre)
add_dependencies(zbasectl customerdirectory)

add_executable(zmaster
    $<TARGET_OBJECTS:stx-mdb>
    $<TARGET_OBJECTS:customerdirectory>
    $<TARGET_OBJECTS:tsdb-server>
    $<TARGET_OBJECTS:coreprotos>
    master/ConfigDirectoryMaster.cc
    master/MasterServlet.cc
    zmaster.cc)

target_link_libraries(zmaster chartsql cplot sstable cstable stx-http stx-protobuf stx-json stx-json stx-base pcre)
add_dependencies(zmaster customerdirectory)

add_executable(zli zli.cc)
target_link_libraries(zli stx-http stx-json stx-protobuf stx-base pcre)

add_executable(zen-csv-upload ${COREPROTO_SRCS} zen-csv-upload.cc)
target_link_libraries(zen-csv-upload stx-http stx-protobuf stx-json stx-base pcre)

add_executable(zen-logfile-upload ${COREPROTO_SRCS} zen-logfile-upload.cc)
target_link_libraries(zen-logfile-upload stx-http stx-protobuf stx-json stx-base pcre)

add_executable(zen-statsd-upload ${COREPROTO_SRCS} zen-statsd-upload.cc)
target_link_libraries(zen-statsd-upload stx-http stx-protobuf stx-json stx-base pcre)

if(MYSQL_FOUND)
  add_executable(zen-mysql-upload
      $<TARGET_OBJECTS:stx-mysql>
      $<TARGET_OBJECTS:tsdb-client>
      ${PROTO_SRCS}
      zen-mysql-upload.cc)

  target_link_libraries(zen-mysql-upload stx-http stx-protobuf stx-json stx-base ${MYSQL_CLIENT_LIBS} z pcre)
endif()
