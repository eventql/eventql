# This file is part of the "libstx" project
#   Copyright (c) 2015 Paul Asmuth
#
# libstx is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License v3.0. You should have received a
# copy of the GNU General Public License along with this program. If not, see
# <http://www.gnu.org/licenses/>.
add_library(stx-mdb OBJECT
    ../3rdparty/liblmdb/mdb.c
    ../3rdparty/liblmdb/midl.c
    MDB.cc
    MDBCursor.cc
    MDBTransaction.cc)
