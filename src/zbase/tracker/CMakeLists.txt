add_executable(ztracker
    $<TARGET_OBJECTS:stx-rpc>
    $<TARGET_OBJECTS:stx-mdb>
    $<TARGET_OBJECTS:brokerd-client>
    $<TARGET_OBJECTS:customerdirectory>
    $<TARGET_OBJECTS:tsdb-server>
    TrackerServlet.cc
    ztracker.cc)

target_link_libraries(ztracker chartsql cplot sstable cstable stx-http stx-protobuf stx-json stx-json stx-base)
