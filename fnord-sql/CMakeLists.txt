# This file is part of the "FnordMetric" project
#   Copyright (c) 2014 Paul Asmuth, Google Inc.
#
# FnordMetric is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License v3.0. You should have received a
# copy of the GNU General Public License along with this program. If not, see
# <http://www.gnu.org/licenses/>.
cmake_minimum_required(VERSION 2.8.8)
project(libfnord-json)
include(FindPkgConfig)

option(ENABLE_TESTS "Build unit tests [default: ON]" ON)

include_directories(${PROJECT_BINARY_DIR})
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
include_directories(../)
include_directories(../3rdparty/)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/")

find_package(MySQL)
if(MYSQL_FOUND)
  add_definitions(-DFNORD_ENABLE_MYSQL=1)
endif()

set(FNORD_SQL_LIBS
    ${CMAKE_THREAD_LIBS_INIT}
    ${MYSQL_CLIENT_LIBS}
    PARENT_SCOPE)

if(APPLE)
  set(CMAKE_CXX_FLAGS "-std=c++0x -stdlib=libc++ ${CMAKE_CXX_FLAGS}")
else()
  set(CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")
endif()

add_library(fnord-sql OBJECT
    MySQL.cc
    MySQLConnection.cc)

