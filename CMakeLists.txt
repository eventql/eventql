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
  set(CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")
endif()

find_package(Threads)

add_subdirectory(src/fnord)

add_library(cm-common OBJECT
    src/itemref.cc
    src/customernamespace.cc
    src/tracker/trackedsession.cc
    src/tracker/trackedquery.cc)

add_executable(cm-frontend
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-cli>
    $<TARGET_OBJECTS:fnord-comm>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-io>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-sstable>
    $<TARGET_OBJECTS:fnord-thread>
    $<TARGET_OBJECTS:fnord-logstream-service>
    $<TARGET_OBJECTS:fnord-util>
    src/cm-frontend.cc
    src/tracker/tracker.cc
    src/tracker/logjoinservice.cc)

add_executable(cm-feedserver
    $<TARGET_OBJECTS:cm-common>
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-cli>
    $<TARGET_OBJECTS:fnord-comm>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-io>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-sstable>
    $<TARGET_OBJECTS:fnord-thread>
    $<TARGET_OBJECTS:fnord-logstream-service>
    $<TARGET_OBJECTS:fnord-util>
    src/cm-feedserver.cc
    src/tracker/tracker.cc
    src/tracker/logjoinservice.cc)

target_link_libraries(cm-feedserver ${CMAKE_THREAD_LIBS_INIT})
