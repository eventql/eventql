/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#ifndef _STX_MDBCURSOR_H
#define _STX_MDBCURSOR_H
#include <memory>
#include <vector>
#include <liblmdb/lmdb.h>
#include "eventql/util/autoref.h"
#include "eventql/util/logging.h"
#include "eventql/util/option.h"

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
