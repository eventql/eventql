add_executable(tests/test-sensord
    $<TARGET_OBJECTS:fnord-base>
    sensord_test.cc)

target_link_libraries(tests/test-sensord ${CMAKE_THREAD_LIBS_INIT} ${FNORD_MSG_LIBS})


