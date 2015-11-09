add_custom_command(
    OUTPUT ztracker_trackjs.cc
    COMMAND 
        java -jar ${CMAKE_SOURCE_DIR}/3rdparty/closure_compiler/compiler.jar  --js ${CMAKE_CURRENT_SOURCE_DIR}/track.js --js_output_file ${CMAKE_CURRENT_BINARY_DIR}/track.min.js --language_in=ECMASCRIPT5 &&
        echo 'zbase/tracker/track.js:${CMAKE_CURRENT_BINARY_DIR}/track.min.js' | ${CMAKE_SOURCE_DIR}/deps/libstx/src/stx/assets.sh ztracker_trackjs.cc
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/track.js")

add_custom_target(ztracker-assets DEPENDS
    "${CMAKE_CURRENT_BINARY_DIR}/ztracker_trackjs.cc")

add_executable(ztracker
    $<TARGET_OBJECTS:stx-rpc>
    $<TARGET_OBJECTS:stx-mdb>
    $<TARGET_OBJECTS:brokerd-client>
    $<TARGET_OBJECTS:customerdirectory>
    $<TARGET_OBJECTS:coreprotos>
    $<TARGET_OBJECTS:tsdb-server>
    ${CMAKE_CURRENT_BINARY_DIR}/ztracker_trackjs.cc
    TrackerServlet.cc
    ztrackid.cc
    ztracker.cc)

target_link_libraries(ztracker chartsql cplot sstable cstable stx-http stx-protobuf stx-json stx-json stx-base pcre)
add_dependencies(ztracker ztracker-assets)
add_dependencies(ztracker customerdirectory)


add_executable(test-ztrackid
    ztrackid.cc
    ztrackid_test.cc)

target_link_libraries(test-ztrackid stx-base pcre)
