add_custom_command(
    OUTPUT ztracker_assets.cc
    COMMAND echo 'zbase/tracker/track.js:${CMAKE_CURRENT_SOURCE_DIR}/track.js' | ${CMAKE_SOURCE_DIR}/src/stx/assets.sh ztracker_assets.cc
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/track.js")

add_custom_target(ztracker-assets DEPENDS
    "${CMAKE_CURRENT_BINARY_DIR}/ztracker_assets.cc")

add_executable(ztracker
    $<TARGET_OBJECTS:stx-rpc>
    $<TARGET_OBJECTS:stx-mdb>
    $<TARGET_OBJECTS:brokerd-client>
    $<TARGET_OBJECTS:customerdirectory>
    $<TARGET_OBJECTS:tsdb-server>
    ${CMAKE_CURRENT_BINARY_DIR}/ztracker_assets.cc
    TrackerServlet.cc
    ztracker.cc)

add_dependencies(ztracker ztracker-assets)
target_link_libraries(ztracker chartsql cplot sstable cstable stx-http stx-protobuf stx-json stx-json stx-base)
