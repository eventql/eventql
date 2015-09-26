add_executable(ztracker
    $<TARGET_OBJECTS:stx-rpc>
    $<TARGET_OBJECTS:customerdirectory>
    ztracker.cc)

target_link_libraries(zmaster stx-http stx-protobuf stx-json stx-json stx-base)
