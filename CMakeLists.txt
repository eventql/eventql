cmake_minimum_required(VERSION 2.8.8)
include(FindPkgConfig)
include(CheckIncludeFile)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/modules/") 
project(cmapp)

option(ENABLE_TESTS "Build unit tests [default: ON]" ON)

include_directories(${PROJECT_BINARY_DIR})
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
include_directories(./src)

if(APPLE)
  set(CMAKE_CXX_FLAGS "-std=c++0x -stdlib=libc++ ${CMAKE_CXX_FLAGS}")
else()
  set(CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS} -lstdc++ -lm")
endif()

find_package(Threads)

add_subdirectory(src/fnord)

add_library(cm-common OBJECT
    src/itemref.cc
    src/customernamespace.cc
    src/tracker/trackedsession.cc
    src/tracker/trackedquery.cc
    src/tracker/tracker.cc)

add_executable(cm-frontend
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-cli>
    $<TARGET_OBJECTS:fnord-comm>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-net>
    $<TARGET_OBJECTS:fnord-stats>
    $<TARGET_OBJECTS:fnord-sstable>
    $<TARGET_OBJECTS:fnord-logstream-service>
    $<TARGET_OBJECTS:fnord-util>
    src/cm-frontend.cc)

target_link_libraries(cm-frontend ${CMAKE_THREAD_LIBS_INIT})

add_executable(cm-feedserver
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-cli>
    $<TARGET_OBJECTS:fnord-comm>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-net>
    $<TARGET_OBJECTS:fnord-stats>
    $<TARGET_OBJECTS:fnord-sstable>
    $<TARGET_OBJECTS:fnord-logstream-service>
    $<TARGET_OBJECTS:fnord-util>
    src/cm-feedserver.cc)

target_link_libraries(cm-feedserver ${CMAKE_THREAD_LIBS_INIT})

add_executable(cm-logjoin
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-cli>
    $<TARGET_OBJECTS:fnord-comm>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-net>
    $<TARGET_OBJECTS:fnord-stats>
    $<TARGET_OBJECTS:fnord-sstable>
    $<TARGET_OBJECTS:fnord-logstream-service>
    $<TARGET_OBJECTS:fnord-util>
    src/cm-logjoin.cc
    src/tracker/logjoin.cc)

target_link_libraries(cm-logjoin ${CMAKE_THREAD_LIBS_INIT})

add_executable(cm-crawlworker
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-cli>
    $<TARGET_OBJECTS:fnord-comm>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-net>
    $<TARGET_OBJECTS:fnord-stats>
    $<TARGET_OBJECTS:fnord-redis>
    $<TARGET_OBJECTS:fnord-sstable>
    $<TARGET_OBJECTS:fnord-logstream-service>
    $<TARGET_OBJECTS:fnord-util>
    src/crawler/crawler.cc
    src/cm-crawlworker.cc)

target_link_libraries(cm-crawlworker ${CMAKE_THREAD_LIBS_INIT})
