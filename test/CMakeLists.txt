if(ENABLE_TESTS)

  # add_executable(itest lifecycle-test.cpp)
  # target_link_libraries(itest xzero)

  add_executable(libxzero-test xzero-test.cpp
    # base
    Base64-test.cpp
    Buffer-test.cpp
    ByteArrayEndPoint-test.cpp
    Cidr-test.cpp
    DirectExecutor-test.cpp
    #IdleTimeout-test.cpp
    ThreadPool-test.cpp
    Tokenizer-test.cpp
    # support
    #LibevScheduler-test.cpp
    # http common
    HeaderFieldList-test.cpp
    HttpRangeDef-test.cpp
    HttpChannel-test.cpp
    # http1
    Http1-test.cpp
    HttpParser-test.cpp
    HttpGenerator-test.cpp
  )
  target_link_libraries(libxzero-test gtest xzero)

  add_custom_target(test-libxzero
    DEPENDS libxzero-test
    COMMAND ./libxzero-test ${CMAKE_CURRENT_SOURCE_DIR}
  )

endif()
