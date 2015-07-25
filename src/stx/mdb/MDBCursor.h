/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_MDBCURSOR_H
#define _STX_MDBCURSOR_H
#include <memory>
#include <vector>
#include <liblmdb/lmdb.h>
#include "stx/autoref.h"
#include "stx/logging.h"
#include "stx/option.h"

namespace stx {
namespace mdb {

class MDBCursor : public RefCounted {
public:

  MDBCursor(MDB_cursor* mdb_cur);
  MDBCursor(const MDBCursor& other) = delete;
  MDBCursor& operator=(const MDBCursor& other) = delete;
  ~MDBCursor();

  bool set(const void* key, size_t key_size);

  void put(
      const void* key,
      size_t key_size,
      const void* value,
      size_t value_size);

  bool get(const String& key, Buffer* value);
  bool get(const Buffer& key, Buffer* value);
  bool get(
      const void* key,
      size_t key_size,
      void** value,
      size_t* value_size);

  bool getFirstOrGreater(Buffer* key, Buffer* value);
  bool getFirstOrGreater(
      void** key,
      size_t* key_size,
      void** value,
      size_t* value_size);

  bool getFirst(Buffer* key, Buffer* value);
  bool getFirst(
      void** key,
      size_t* key_size,
      void** value,
      size_t* value_size);

  bool getLast(Buffer* key, Buffer* value);
  bool getLast(
      void** key,
      size_t* key_size,
      void** value,
      size_t* value_size);

  bool getNext(Buffer* key, Buffer* value);
  bool getNext(
      void** key,
      size_t* key_size,
      void** value,
      size_t* value_size);

  bool getPrev(Buffer* key, Buffer* value);
  bool getPrev(
      void** key,
      size_t* key_size,
      void** value,
      size_t* value_size);


  void close();

  void del();

protected:
  MDB_cursor* mdb_cur_;
  bool is_closed_;
};

}
}
#endif
