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

# base
add_library(stx-base STATIC
    $<TARGET_OBJECTS:pcre>
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
    web/SecureCookie.cc)

file(GLOB PROTO_FILES *.proto)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})
set(PROTOC_ARGS --proto_path ${CMAKE_CURRENT_SOURCE_DIR}/ PARENT_SCOPE)

add_library(stx-protobuf STATIC
    ../../3rdparty/google/protobuf/compiler/parser.cc
    ../../3rdparty/google/protobuf/descriptor.cc
    ../../3rdparty/google/protobuf/descriptor.pb.cc
    ../../3rdparty/google/protobuf/descriptor_database.cc
    ../../3rdparty/google/protobuf/dynamic_message.cc
    ../../3rdparty/google/protobuf/extension_set.cc
    ../../3rdparty/google/protobuf/extension_set_heavy.cc
    ../../3rdparty/google/protobuf/generated_message_reflection.cc
    ../../3rdparty/google/protobuf/generated_message_util.cc
    ../../3rdparty/google/protobuf/message.cc
    ../../3rdparty/google/protobuf/message_lite.cc
    ../../3rdparty/google/protobuf/reflection_ops.cc
    ../../3rdparty/google/protobuf/repeated_field.cc
    ../../3rdparty/google/protobuf/service.cc
    ../../3rdparty/google/protobuf/text_format.cc
    ../../3rdparty/google/protobuf/unknown_field_set.cc
    ../../3rdparty/google/protobuf/wire_format.cc
    ../../3rdparty/google/protobuf/wire_format_lite.cc
    ../../3rdparty/google/protobuf/io/coded_stream.cc
    ../../3rdparty/google/protobuf/io/gzip_stream.cc
    ../../3rdparty/google/protobuf/io/printer.cc
    ../../3rdparty/google/protobuf/io/tokenizer.cc
    ../../3rdparty/google/protobuf/io/zero_copy_stream.cc
    ../../3rdparty/google/protobuf/io/zero_copy_stream_impl.cc
    ../../3rdparty/google/protobuf/io/zero_copy_stream_impl_lite.cc
    ../../3rdparty/google/protobuf/stubs/atomicops_internals_x86_gcc.cc
    ../../3rdparty/google/protobuf/stubs/atomicops_internals_x86_msvc.cc
    ../../3rdparty/google/protobuf/stubs/common.cc
    ../../3rdparty/google/protobuf/stubs/once.cc
    ../../3rdparty/google/protobuf/stubs/stringprintf.cc
    ../../3rdparty/google/protobuf/stubs/structurally_valid.cc
    ../../3rdparty/google/protobuf/stubs/strutil.cc
    ../../3rdparty/google/protobuf/stubs/substitute.cc
    Currency.cc # FIXME
    protobuf/MessageDecoder.cc
    protobuf/MessageEncoder.cc
    protobuf/MessagePrinter.cc
    protobuf/MessageObject.cc
    protobuf/MessageSchema.cc
    protobuf/DynamicMessage.cc
    protobuf/JSONEncoder.cc
    ${PROTO_SRCS})

add_executable(test-inputstream io/inputstream_test.cc)
target_link_libraries(test-inputstream stx-base)

add_executable(test-uri uri_test.cc)
target_link_libraries(test-uri stx-base)

add_executable(test-stringutil stringutil_test.cc)
target_link_libraries(test-stringutil stx-base)

add_executable(test-internmap InternMap_test.cc)
target_link_libraries(test-internmap stx-base)

add_executable(test-hmac HMAC_test.cc)
target_link_libraries(test-hmac stx-base)

add_executable(test-human human_test.cc)
target_link_libraries(test-human stx-base)

add_executable(test-secure-cookie web/SecureCookie_test.cc)
target_link_libraries(test-secure-cookie stx-base)

add_executable(test-sha1 SHA1_test.cc)
target_link_libraries(test-sha1 stx-base)

add_executable(test-protobuf
    $<TARGET_OBJECTS:stx-json>
    $<TARGET_OBJECTS:stx-http>
    protobuf/protobuf_test.cc)
target_link_libraries(test-protobuf stx-base stx-protobuf)

add_executable(test-persistenthashset util/PersistentHashSet_test.cc)
target_link_libraries(test-persistenthashset stx-base)

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
#    
#    $<TARGET_OBJECTS:fnord-comm>
#    $<TARGET_OBJECTS:fnord-net>
#    $<TARGET_OBJECTS:fnord-redis>
#    net/redis/redis_test.cc)
