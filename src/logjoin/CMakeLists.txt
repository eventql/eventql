#file(GLOB PROTO_FILES *.proto)
#PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

add_library(logjoin-common OBJECT
    common.cc
    ${PROTO_SRCS})

add_dependencies(logjoin-common customerdirectory)

add_library(logjoin-base OBJECT
    common.cc
    ${PROTO_SRCS}
    TrackedCartItem.cc
    TrackedItemVisit.cc
    TrackedQuery.cc
    TrackedSession.cc
    SessionContext.cc
    LogJoin.cc
    LogJoinExport.cc
    SessionProcessor.cc
    stages/SessionJoin.cc
    stages/BuildSessionAttributes.cc
    stages/NormalizeQueryStrings.cc
    stages/DebugPrintStage.cc
    stages/TSDBUploadStage.cc
    stages/DeliverWebhookStage.cc
    LogJoinShard.cc)

add_dependencies(logjoin-base customerdirectory)

add_executable(logjoind
    $<TARGET_OBJECTS:stx-base>
    $<TARGET_OBJECTS:stx-http>
    $<TARGET_OBJECTS:stx-json>
    $<TARGET_OBJECTS:sstable>
    $<TARGET_OBJECTS:brokerd-client>
    $<TARGET_OBJECTS:inventoryd-client>
    $<TARGET_OBJECTS:stx-mdb>
    $<TARGET_OBJECTS:stx-rpc>
    $<TARGET_OBJECTS:stx-protobuf>
    $<TARGET_OBJECTS:fnord-fts>
    $<TARGET_OBJECTS:hunspell>
    $<TARGET_OBJECTS:tsdb-client>
    $<TARGET_OBJECTS:dproc>
    $<TARGET_OBJECTS:libcstable>
    $<TARGET_OBJECTS:customerdirectory>
    $<TARGET_OBJECTS:logjoin-base>
    logjoind.cc)

target_link_libraries(logjoind ${CMAKE_THREAD_LIBS_INIT} ${FNORD_LIBS} ${FNORD_FTS_LIBS})

add_executable(test-logjoin
    $<TARGET_OBJECTS:stx-base>
    $<TARGET_OBJECTS:stx-http>
    $<TARGET_OBJECTS:stx-json>
    $<TARGET_OBJECTS:sstable>
    $<TARGET_OBJECTS:brokerd-client>
    $<TARGET_OBJECTS:inventoryd-client>
    $<TARGET_OBJECTS:stx-mdb>
    $<TARGET_OBJECTS:stx-rpc>
    $<TARGET_OBJECTS:stx-protobuf>
    $<TARGET_OBJECTS:fnord-fts>
    $<TARGET_OBJECTS:hunspell>
    $<TARGET_OBJECTS:tsdb-client>
    $<TARGET_OBJECTS:dproc>
    $<TARGET_OBJECTS:libcstable>
    $<TARGET_OBJECTS:customerdirectory>
    $<TARGET_OBJECTS:logjoin-base>
    LogJoin_test.cc)

target_link_libraries(test-logjoin ${CMAKE_THREAD_LIBS_INIT} ${FNORD_LIBS} ${FNORD_FTS_LIBS})

#add_executable(cm-logjoin-backfill
#    $<TARGET_OBJECTS:cm-common>
#    $<TARGET_OBJECTS:stx-base>
#    $<TARGET_OBJECTS:fnord-feeds>
#    $<TARGET_OBJECTS:stx-mdb>
#    $<TARGET_OBJECTS:stx-http>
#    $<TARGET_OBJECTS:stx-json>
#    $<TARGET_OBJECTS:stx-rpc>
#    $<TARGET_OBJECTS:libcstable>
#    $<TARGET_OBJECTS:sstable>
#    $<TARGET_OBJECTS:fnord-fts>
#    $<TARGET_OBJECTS:stx-protobuf>
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
