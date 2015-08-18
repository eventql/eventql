/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_MDBTRANSACTION_H
#define _STX_MDBTRANSACTION_H
#include <memory>
#include <vector>
#include <liblmdb/lmdb.h>
#include "stx/autoref.h"
#include "stx/logging.h"
#include "stx/option.h"
#include "stx/mdb/MDBCursor.h"

namespace stx {
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
