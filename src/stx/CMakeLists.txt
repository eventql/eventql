# This file is part of the "libstx" project
#   Copyright (c) 2014 Paul Asmuth, Google Inc.
#   Copyright (c) 2014 Christian Parpart
#
# libstx is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License v3.0. You should have received a
# copy of the GNU General Public License along with this program. If not, see
# <http://www.gnu.org/licenses/>.
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/sysconfig.h.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/sysconfig.h)

file(GLOB PROTO_FILES *.proto)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

# base
add_library(stx-base OBJECT
    ../../3rdparty/simdcomp/simdbitpacking.c
    ../../3rdparty/simdcomp/simdintegratedbitpacking.c
    ../../3rdparty/simdcomp/simdcomputil.c
    application.cc
    autoref.cc
    assets.cc
    buffer.cc
    bufferutil.cc
    cli/flagparser.cc
    cli/CLI.cc
    cli/CLICommand.cc
    csv/CSVOutputStream.cc
    csv/BinaryCSVOutputStream.cc
    csv/CSVInputStream.cc
    csv/BinaryCSVInputStream.cc
    UnixTime.cc
    duration.cc
    exception.cc
    exceptionhandler.cc
    fnv.cc
    HMAC.cc
    human.cc
    ieee754.cc
    inspect.cc
    InternMap.cc
    io/file.cc
    io/FileLock.cc
    io/fileutil.cc
    io/filerepository.cc
    io/inputstream.cc
    io/outputstream.cc
    io/BufferedOutputStream.cc
    io/mmappedfile.cc
    io/pagemanager.cc
    ISO8601.cc
    Currency.cc
    CivilTime.cc
    Language.cc
    logging.cc
    logging/logoutputstream.cc
    net/dnscache.cc
    net/tcpserver.cc
    net/udpserver.cc
    net/udpsocket.cc
    net/inetaddr.cc
    net/tcpconnection.cc
    random.cc
    RegExp.cc
    rpc/RPCRequest.cc
    rpc/RPCContext.cc
    rpc/RPCStub.cc
    rpc/RPCService.cc
    SHA1.cc
    StackTrace.cc
    status.cc
    stats/statsdagent.cc
    stats/statsrepository.cc
    stats/statssink.cc
    stats/statsd.cc
    stringutil.cc
    thread/eventloop.cc
    thread/signalhandler.cc
    thread/threadpool.cc
    thread/FixedSizeThreadPool.cc
    thread/wakeup.cc
    uri.cc
    UTF8.cc
    util/Base64.cc
    util/binarymessagereader.cc
    util/binarymessagewriter.cc
    util/CumulativeHistogram.cc
    util/BitPackDecoder.cc
    util/BitPackEncoder.cc
    util/SimpleRateLimit.cc
    util/PersistentHashSet.cc
    VFS.cc
    wallclock.cc
    web/SecureCookie.cc
    ${PROTO_SRCS})

add_executable(test-inputstream
    $<TARGET_OBJECTS:stx-base>
    io/inputstream_test.cc)

add_executable(test-uri
    $<TARGET_OBJECTS:stx-base>
    uri_test.cc)

add_executable(test-stringutil
    $<TARGET_OBJECTS:stx-base>
    stringutil_test.cc)

add_executable(test-internmap
    $<TARGET_OBJECTS:stx-base>
    InternMap_test.cc)

add_executable(test-hmac
    $<TARGET_OBJECTS:stx-base>
    HMAC_test.cc)

add_executable(test-human
    $<TARGET_OBJECTS:stx-base>
    human_test.cc)

add_executable(test-secure-cookie
    $<TARGET_OBJECTS:stx-base>
    web/SecureCookie_test.cc)

add_executable(test-sha1
    $<TARGET_OBJECTS:stx-base>
    SHA1_test.cc)

add_executable(test-protobuf
    $<TARGET_OBJECTS:stx-base>
    $<TARGET_OBJECTS:stx-protobuf>
    $<TARGET_OBJECTS:stx-json>
    $<TARGET_OBJECTS:stx-http>
    protobuf/protobuf_test.cc)

add_executable(test-persistenthashset
    $<TARGET_OBJECTS:stx-base>
    util/PersistentHashSet_test.cc)

target_link_libraries(test-persistenthashset ${FNORD_LIBS})

## redis
#add_library(fnord-redis OBJECT
#    ../3rdparty/hiredis/async.c
#    ../3rdparty/hiredis/dict.c
#    ../3rdparty/hiredis/hiredis.c
#    ../3rdparty/hiredis/net.c
#    ../3rdparty/hiredis/sds.c
#    net/redis/redisconnection.cc
#    net/redis/redisqueue.cc)

#add_executable(tests/test-redis
#    $<TARGET_OBJECTS:stx-base>
#    $<TARGET_OBJECTS:fnord-comm>
#    $<TARGET_OBJECTS:fnord-net>
#    $<TARGET_OBJECTS:fnord-redis>
#    net/redis/redis_test.cc)
