find_package(Threads)
include(CheckIncludeFiles)
include(CheckIncludeFileCXX)
include(CheckFunctionExists)
include(CheckVariableExists)
include(CheckTypeSize)
include(CheckLibraryExists)
include(CheckCSourceCompiles)
include(CMakeDetermineCCompiler)

CHECK_INCLUDE_FILES(sys/sendfile.h HAVE_SYS_SENDFILE_H)
CHECK_FUNCTION_EXISTS(sendfile HAVE_SENDFILE)
CHECK_FUNCTION_EXISTS(posix_fadvise HAVE_POSIX_FADVISE)
CHECK_FUNCTION_EXISTS(readahead HAVE_READAHEAD)
CHECK_FUNCTION_EXISTS(pread HAVE_PREAD)

CHECK_INCLUDE_FILES(unistd.h HAVE_UNISTD_H)
CHECK_INCLUDE_FILES(sys/inotify.h HAVE_SYS_INOTIFY_H)
CHECK_INCLUDE_FILES(sys/resource.h HAVE_SYS_RESOURCE_H)
CHECK_INCLUDE_FILES(sys/mman.h HAVE_SYS_MMAN_H)
CHECK_INCLUDE_FILES(sys/limits.h HAVE_SYS_LIMITS_H)
CHECK_INCLUDE_FILES(pwd.h HAVE_PWD_H)
CHECK_INCLUDE_FILES(syslog.h HAVE_SYSLOG_H)
CHECK_INCLUDE_FILES(dlfcn.h HAVE_DLFCN_H)
CHECK_INCLUDE_FILES(execinfo.h HAVE_EXECINFO_H)
CHECK_INCLUDE_FILES(uuid/uuid.h HAVE_UUID_UUID_H)

CHECK_FUNCTION_EXISTS(nanosleep HAVE_NANOSLEEP)
CHECK_FUNCTION_EXISTS(daemon HAVE_DAEMON)
CHECK_FUNCTION_EXISTS(sysconf HAVE_SYSCONF)
CHECK_FUNCTION_EXISTS(pathconf HAVE_PATHCONF)
CHECK_FUNCTION_EXISTS(accept4 HAVE_ACCEPT4)
CHECK_FUNCTION_EXISTS(pipe2 HAVE_PIPE2)
CHECK_FUNCTION_EXISTS(dup2 HAVE_DUP2)
CHECK_FUNCTION_EXISTS(dladdr HAVE_DLADDR)
CHECK_FUNCTION_EXISTS(fork HAVE_FORK)
CHECK_FUNCTION_EXISTS(backtrace HAVE_BACKTRACE)
#CHECK_FUNCTION_EXISTS(clock_gettime HAVE_CLOCK_GETTIME)

CHECK_INCLUDE_FILES(pthread.h HAVE_PTHREAD_H)
CHECK_LIBRARY_EXISTS(pthread pthread_setname_np "" HAVE_PTHREAD_SETNAME_NP)
CHECK_LIBRARY_EXISTS(pthread pthread_setaffinity_np "" HAVE_PTHREAD_SETAFFINITY_NP)
CHECK_FUNCTION_EXISTS("gethostbyname_r" HAVE_GETHOSTBYNAME_R)

CHECK_LIBRARY_EXISTS(stdc++ __cxa_pure_virtual "" COMPILER_RT_HAS_LIBSTDCXX)
CHECK_LIBRARY_EXISTS(m pow "" COMPILER_RT_HAS_LIBM)
CHECK_LIBRARY_EXISTS(dl dlopen "" COMPILER_RT_HAS_LIBDL)
CHECK_LIBRARY_EXISTS(pthread pthread_create "" COMPILER_RT_HAS_LIBPTHREAD)

if(NOT COMPILER_RT_HAS_LIBSTDCXX)
  message(FATAL_ERROR "libstdc++ not found")
endif()

include(util/cmake/FindCXX11HashMapAndSet.cmake)

CHECK_INCLUDE_FILES(sys/inotify.h HAVE_SYS_INOTIFY_H)
if(HAVE_SYS_INOTIFY_H)
  CHECK_FUNCTION_EXISTS(inotify_init1 HAVE_INOTIFY_INIT1)
endif(HAVE_SYS_INOTIFY_H)

CHECK_INCLUDE_FILES(zlib.h HAVE_ZLIB_H)
if(HAVE_ZLIB_H)
  CHECK_LIBRARY_EXISTS(z deflate "" HAVE_LIBZ)
  set(ZLIB_LIBRARIES z)
endif(HAVE_ZLIB_H)

CHECK_INCLUDE_FILES(bzlib.h HAVE_BZLIB_H)
if(HAVE_BZLIB_H)
  CHECK_LIBRARY_EXISTS(bz2 BZ2_bzCompressInit "" HAVE_LIBBZ2)
  set(BZIP2_LIBRARIES bz2)
endif(HAVE_BZLIB_H)

option(ENABLE_PCRE "With PCRE support [default: on]" ON)
if(NOT HAVE_PCRE)
  if(ENABLE_PCRE)
    find_package(PCRE)
    if (PCRE_FOUND)
      set(HAVE_PCRE 1)
      message(STATUS "PCRE include dir: ${PCRE_INCLUDE_DIR}")
      include_directories(${PCRE_INCLUDE_DIR})
      set(HAVE_PCRE 1 PARENT_SCOPE)
      set(PCRE_INCLUDE_DIR ${PCRE_INCLUDE_DIR} PARENT_SCOPE)
      set(PCRE_LIBRARIES ${PCRE_LIBRARIES} PARENT_SCOPE)
    endif()
  endif()
endif()

if (HAVE_PCRE)
  message(STATUS "PCRE found")
else()
  message(STATUS "PCRE not found")
endif()

# required for shm_open/shm_unlink
if(APPLE)
  set(RT_LIBRARIES)
else()
  set(RT_LIBRARIES rt)
endif()

if(OPENSSL_FOUND)
  message(STATUS "OpenSSL found")
else()
  message(STATUS "OpenSSL not found")
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/buildconfig.h.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/buildconfig.h)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/sysconfig.h.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/sysconfig.h)

add_subdirectory(util/mdb)

file(GLOB PROTO_FILES *.proto db/*.proto config/*.proto)
STX_PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})
#
#add_executable(test-tsdb-timewindowpartitioner
#    $<TARGET_OBJECTS:stx-mdb>
#    $<TARGET_OBJECTS:evql-core>
#    $<TARGET_OBJECTS:coreprotos>
#    $<TARGET_OBJECTS:customerdirectory>
#    core/TimeWindowPartitioner_test.cc)
#
#target_link_libraries(test-tsdb-timewindowpartitioner chartsql cplot sstable cstable stx-http stx-protobuf stx-json stx-base pcre)
#
#add_executable(test-replicationscheme
#    $<TARGET_OBJECTS:stx-mdb>
#    $<TARGET_OBJECTS:evql-core>
#    $<TARGET_OBJECTS:coreprotos>
#    $<TARGET_OBJECTS:customerdirectory>
#    core/ReplicationScheme_test.cc)
#
#target_link_libraries(test-replicationscheme chartsql cplot sstable cstable stx-http stx-protobuf stx-json stx-base pcre)
#
#add_executable(test-lsmtableindex
#    $<TARGET_OBJECTS:stx-mdb>
#    $<TARGET_OBJECTS:evql-core>
#    $<TARGET_OBJECTS:coreprotos>
#    $<TARGET_OBJECTS:customerdirectory>
#    core/LSMTableIndex_test.cc)
#
#target_link_libraries(test-lsmtableindex chartsql cplot sstable cstable stx-http stx-protobuf stx-json stx-base pcre)
#
#add_subdirectory(util/mysql)
#add_subdirectory(util/mdb)
#
add_custom_command(
    OUTPUT mapreduce_preludejs.cc
    COMMAND 
        echo 'eventql/mapreduce/prelude.js:${CMAKE_CURRENT_SOURCE_DIR}/mapreduce/prelude.js' | ${CMAKE_SOURCE_DIR}/src/eventql/util/assets.sh mapreduce_preludejs.cc
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/mapreduce/prelude.js")
#
##add_executable(test-javascriptcontext
##    $<TARGET_OBJECTS:stx-mdb>
##    $<TARGET_OBJECTS:evql-core>
##    $<TARGET_OBJECTS:coreprotos>
##    $<TARGET_OBJECTS:customerdirectory>
##    mapreduce/MapReduceTask.cc
##    mapreduce/MapReduceTaskBuilder.cc
##    mapreduce/MapReduceScheduler.cc
##    mapreduce/tasks/MapTableTask.cc
##    mapreduce/tasks/ReduceTask.cc
##    mapreduce/tasks/ReturnResultsTask.cc
##    mapreduce/tasks/SaveToTableTask.cc
##    mapreduce/tasks/SaveToTablePartitionTask.cc
##    transport/http/MapReduceService.cc
##    JavaScriptContext.cc
##    JavaScriptContext_test.cc)
##
##target_link_libraries(test-javascriptcontext chartsql cplot sstable cstable stx-http stx-protobuf stx-json stx-base pcre z)
##target_link_libraries(test-javascriptcontext -L${CMAKE_BINARY_DIR}/deps/3rdparty/mozjs/dist/lib -ljs_static)



add_executable(evqld
    #eventql/io/sstable/SSTableServlet.cc
    ${CMAKE_CURRENT_BINARY_DIR}/mapreduce_preludejs.cc
    ${PROTO_SRCS}
    AnalyticsApp.cc
    AnalyticsAuth.cc
    transport/http/api_servlet.cc
    config/config_directory.cc
    config/namespace_config.cc
    transport/http/default_servlet.cc
    EventScanResult.cc
    transport/http/http_auth.cc
    JavaScriptContext.cc
    LogfileScanResult.cc
    transport/http/status_servlet.cc
    TableDefinition.cc
    transport/http/EventsAPIServlet.cc
    transport/http/EventsService.cc
    transport/http/LogfileAPIServlet.cc
    transport/http/LogfileService.cc
    transport/http/MapReduceAPIServlet.cc
    transport/http/MapReduceService.cc
    db/CompactionStrategy.cc
    db/CompactionWorker.cc
    db/FixedShardPartitioner.cc
    db/LSMPartitionReader.cc
    db/LSMPartitionReplication.cc
    db/LSMPartitionWriter.cc
    db/LSMTableIndex.cc
    db/LSMTableIndexCache.cc
    db/LazyPartition.cc
    db/LogPartitionReader.cc
    db/LogPartitionReplication.cc
    db/LogPartitionWriter.cc
    db/Partition.cc
    db/partition_map.cc
    db/PartitionReader.cc
    db/PartitionReplication.cc
    db/PartitionSnapshot.cc
    db/PartitionWriter.cc
    db/RecordArena.cc
    db/RecordSet.cc
    db/ReplicationScheme.cc
    db/ReplicationState.cc
    db/ReplicationWorker.cc
    db/StaticPartitionReader.cc
    db/StaticPartitionReplication.cc
    db/StaticPartitionWriter.cc
    db/TSDBClient.cc
    db/TSDBService.cc
    db/TSDBServlet.cc
    db/TSDBTableRef.cc
    db/Table.cc
    db/TimeWindowPartitioner.cc
    mapreduce/MapReduceScheduler.cc
    mapreduce/MapReduceTask.cc
    mapreduce/MapReduceTaskBuilder.cc
    mapreduce/tasks/MapTableTask.cc
    mapreduce/tasks/ReduceTask.cc
    mapreduce/tasks/ReturnResultsTask.cc
    mapreduce/tasks/SaveToTablePartitionTask.cc
    mapreduce/tasks/SaveToTableTask.cc
    server/sql/codec/binary_codec.cc
    server/sql/codec/json_codec.cc
    server/sql/sql_engine.cc
    server/sql/table_scan.cc
    server/sql/table_provider.cc
    server/sql/partition_cursor.cc
    server/sql/remote_expression.cc
    server/sql/scheduler.cc
    server/sql/transaction_info.cc
    z1stats.cc
    evqld.cc)

target_link_libraries(evqld chartsql cplot sstable cstable stx-http stx-protobuf stx-json stx-base stx-rpc stx-mdb js_static.a z pcre)
target_link_libraries(evqld spidermonkey)

add_executable(evql
    cli/console.cc
    server/sql/codec/binary_codec.cc
    evql.cc)

target_link_libraries(evql chartsql stx-http stx-protobuf stx-json stx-base stx-rpc z pcre)
target_link_libraries(evql editline)

#add_executable(evqlctl
#    $<TARGET_OBJECTS:stx-mdb>
#    $<TARGET_OBJECTS:evql-core>
#    $<TARGET_OBJECTS:coreprotos>
#    $<TARGET_OBJECTS:customerdirectory>
#    #$<TARGET_OBJECTS:fnord-afx>
#    #eventql/io/sstable/SSTableServlet.cc
#    evqlctl.cc)
#
#target_link_libraries(evqlctl chartsql cplot sstable cstable stx-http stx-protobuf stx-json stx-base stx-rpc pcre)
#add_dependencies(evqlctl customerdirectory)
#
#add_executable(zmaster
#    $<TARGET_OBJECTS:stx-mdb>
#    $<TARGET_OBJECTS:customerdirectory>
#    $<TARGET_OBJECTS:evql-core>
#    $<TARGET_OBJECTS:coreprotos>
#    master/ConfigDirectoryMaster.cc
#    master/MasterServlet.cc
#    zmaster.cc)
#
#add_executable(evql
#    server/sql/codec/binary_codec.cc
#    evql.cc)
#
#target_link_libraries(evql chartsql cplot cstable stx-http stx-protobuf stx-json stx-base pcre)
#
#target_link_libraries(zmaster chartsql cplot sstable cstable stx-http stx-protobuf stx-json stx-json stx-base pcre)
#add_dependencies(zmaster customerdirectory)
