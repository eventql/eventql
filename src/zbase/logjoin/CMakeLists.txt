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
    stages/BuildEventsStage.cc
    stages/BuildSessionAttributes.cc
    stages/NormalizeQueryStrings.cc
    stages/DebugPrintStage.cc
    stages/TSDBUploadStage.cc
    stages/DeliverWebhookStage.cc
    LogJoinShard.cc)

add_dependencies(logjoin-base customerdirectory)

add_executable(zlogjoin
    $<TARGET_OBJECTS:brokerd-client>
    $<TARGET_OBJECTS:inventoryd-client>
    $<TARGET_OBJECTS:stx-mdb>
    $<TARGET_OBJECTS:stx-rpc>
    $<TARGET_OBJECTS:nlputil>
    $<TARGET_OBJECTS:hunspell>
    $<TARGET_OBJECTS:tsdb-client>
    $<TARGET_OBJECTS:dproc>
    $<TARGET_OBJECTS:customerdirectory>
    $<TARGET_OBJECTS:logjoin-base>
    zlogjoin.cc)

target_link_libraries(zlogjoin stx-http stx-protobuf stx-json sstable cstable chartsql cplot stx-base)

add_executable(test-logjoin
    $<TARGET_OBJECTS:brokerd-client>
    $<TARGET_OBJECTS:inventoryd-client>
    $<TARGET_OBJECTS:stx-mdb>
    $<TARGET_OBJECTS:stx-rpc>
    $<TARGET_OBJECTS:nlputil>
    $<TARGET_OBJECTS:hunspell>
    $<TARGET_OBJECTS:tsdb-client>
    $<TARGET_OBJECTS:dproc>
    $<TARGET_OBJECTS:customerdirectory>
    $<TARGET_OBJECTS:logjoin-base>
    LogJoin_test.cc)

target_link_libraries(test-logjoin stx-http stx-protobuf stx-json sstable cstable chartsql cplot stx-base)
