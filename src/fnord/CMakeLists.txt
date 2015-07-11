# This file is part of the "FnordMetric" project
#   Copyright (c) 2014 Paul Asmuth, Google Inc.
#
# FnordMetric is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License v3.0. You should have received a
# copy of the GNU General Public License along with this program. If not, see
# <http://www.gnu.org/licenses/>.
file(GLOB PROTO_FILES *.proto)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

# base
add_library(fnord-base OBJECT
    ../../3rdparty/libart/art.c
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
    io/mmappedfile.cc
    io/pagemanager.cc
    Currency.cc
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
    util/CSVOutputStream.cc
    util/CumulativeHistogram.cc
    util/BitPackDecoder.cc
    util/BitPackEncoder.cc
    util/SimpleRateLimit.cc
    VFS.cc
    wallclock.cc
    web/SecureCookie.cc
    ${PROTO_SRCS})

add_executable(tests/test-inputstream
    $<TARGET_OBJECTS:fnord-base>
    io/inputstream_test.cc)

add_executable(tests/test-uri
    $<TARGET_OBJECTS:fnord-base>
    uri_test.cc)

add_executable(tests/test-stringutil
    $<TARGET_OBJECTS:fnord-base>
    stringutil_test.cc)

add_executable(tests/test-internmap
    $<TARGET_OBJECTS:fnord-base>
    InternMap_test.cc)

add_executable(tests/test-hmac
    $<TARGET_OBJECTS:fnord-base>
    HMAC_test.cc)

add_executable(tests/test-secure-cookie
    $<TARGET_OBJECTS:fnord-base>
    web/SecureCookie_test.cc)

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
#    $<TARGET_OBJECTS:fnord-base>
#    $<TARGET_OBJECTS:fnord-comm>
#    $<TARGET_OBJECTS:fnord-net>
#    $<TARGET_OBJECTS:fnord-redis>
#    net/redis/redis_test.cc)
