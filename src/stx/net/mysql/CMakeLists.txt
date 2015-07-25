# This file is part of the "libstx" project
#   Copyright (c) 2014 Paul Asmuth, Google Inc.
#
# libstx is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License v3.0. You should have received a
# copy of the GNU General Public License along with this program. If not, see
# <http://www.gnu.org/licenses/>.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/") 

find_package(MySQL)

if(MYSQL_FOUND)
  add_definitions(-DSTX_ENABLE_MYSQL)
  set(STX_MYSQL_LIBS ${MYSQL_CLIENT_LIBS} PARENT_SCOPE)
else()
  message("WARNING: libmysqlclient not found, libstx will be compiled without MySQL support")
endif()

add_library(stx-mysql OBJECT
    MySQLConnection.cc
    MySQL.cc)
