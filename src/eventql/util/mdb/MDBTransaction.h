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
#ifndef _STX_MDBTRANSACTION_H
#define _STX_MDBTRANSACTION_H
#include <memory>
#include <vector>
#include <liblmdb/lmdb.h>
#include "eventql/util/autoref.h"
#include "eventql/util/logging.h"
#include "eventql/util/option.h"
#include "eventql/util/mdb/MDBCursor.h"

namespace util {
namespace mdb {
struct MDBOptions;

class MDBTransaction : public RefCounted {
public:

  MDBTransaction(
      MDB_txn* mdb_txn,
      MDB_dbi mdb_handle,
      MDBOptions* opts);

  ~MDBTransaction();
  MDBTransaction(const MDBTransaction& other) = delete;
  MDBTransaction& operator=(const MDBTransaction& other) = delete;

  RefPtr<MDBCursor> getCursor();

  void commit();
  void abort();
  void autoAbort();

  Option<Buffer> get(const Buffer& key);
  Option<Buffer> get(const String& key);
  bool get(
      const void* key,
      size_t key_size,
      void** value,
      size_t* value_size);

  void insert(const String& key, const String& value);
  void insert(const String& key, const Buffer& value);
  void insert(const String& key, const void* value, size_t value_size);
  void insert(const Buffer& key, const Buffer& value);
  void insert(const Buffer& key, const void* value, size_t value_size);
  void insert(
      const void* key,
      size_t key_size,
      const void* value,
      size_t value_size);

  void update(const String& key, const String& value);
  void update(const String& key, const Buffer& value);
  void update(const String& key, const void* value, size_t value_size);
  void update(const Buffer& key, const Buffer& value);
  void update(const Buffer& key, const void* value, size_t value_size);
  void update(
      const void* key,
      size_t key_size,
      const void* value,
      size_t value_size);

  void del(const String& key);
  void del(const Buffer& key);
  void del(const void* key, size_t key_size);

protected:
  MDB_txn* mdb_txn_;
  MDB_dbi mdb_handle_;
  MDBOptions* opts_;
  bool is_commited_;
  bool abort_on_free_;
};

}
}
#endif
