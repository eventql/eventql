/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <assert.h>
#include "stx/mdb/MDB.h"
#include "stx/mdb/MDBTransaction.h"

namespace stx {
namespace mdb {

MDBTransaction::MDBTransaction(
    MDB_txn* mdb_txn,
    MDB_dbi mdb_handle,
    MDBOptions* opts) :
    mdb_txn_(mdb_txn),
    mdb_handle_(mdb_handle),
    opts_(opts),
    is_commited_(false),
    abort_on_free_(false) {}

MDBTransaction::~MDBTransaction() {
  if (!is_commited_ && abort_on_free_) {
    mdb_txn_abort(mdb_txn_);
  }
}

RefPtr<MDBCursor> MDBTransaction::getCursor() {
  MDB_cursor* cursor;

  auto rc = mdb_cursor_open(mdb_txn_, mdb_handle_, &cursor);
  if (rc != 0) {
    auto err = String(mdb_strerror(rc));
    RAISEF(kRuntimeError, "mdb_cusor_open() failed: $0", err);
  }

  return RefPtr<MDBCursor>(new MDBCursor(cursor));
}

void MDBTransaction::commit() {
  if (is_commited_) {
    RAISE(kRuntimeError, "transaction was already commited or aborted");
  }

  is_commited_ = true;

  auto rc = mdb_txn_commit(mdb_txn_);
  if (rc != 0) {
    auto err = String(mdb_strerror(rc));
    RAISEF(kRuntimeError, "mdb_txn_commit() failed: $0", err);
  }
}

void MDBTransaction::abort() {
  if (is_commited_) {
    RAISE(kRuntimeError, "transaction was already commited or aborted");
  }

  is_commited_ = true;
  mdb_txn_abort(mdb_txn_);
}

void MDBTransaction::autoAbort() {
  abort_on_free_ = true;
}

Option<Buffer> MDBTransaction::get(const Buffer& key) {
  void* data;
  size_t data_size;

  if (get(key.data(), key.size(), &data, &data_size)) {
    return Buffer(data, data_size);
  } else {
    return None<Buffer>();
  }
}

Option<Buffer> MDBTransaction::get(const String& key) {
  void* data;
  size_t data_size;

  if (get(key.c_str(), key.length(), &data, &data_size)) {
    return Some(Buffer(data, data_size));
  } else {
    return None<Buffer>();
  }
}

bool MDBTransaction::get(
    const void* key,
    size_t key_size,
    void** value,
    size_t* value_size) {
  MDB_val mkey, mval;
  mkey.mv_data = const_cast<void*>(key);
  mkey.mv_size = key_size;

  auto rc = mdb_get(mdb_txn_, mdb_handle_, &mkey, &mval);

  if (rc == MDB_NOTFOUND) {
    return false;
  }

  if (rc != 0) {
    auto err = String(mdb_strerror(rc));
    RAISEF(kRuntimeError, "mdb_get() failed: $0", err);
  }

  *value = mval.mv_data;
  *value_size = mval.mv_size;
  return true;
}

void MDBTransaction::insert(const String& key, const String& value) {
  insert(key.c_str(), key.length(), value.c_str(), value.length());
}

void MDBTransaction::insert(const String& key, const Buffer& value) {
  insert(key.c_str(), key.length(), value.data(), value.size());
}

void MDBTransaction::insert(const Buffer& key, const Buffer& value) {
  insert(key.data(), key.size(), value.data(), value.size());
}

void MDBTransaction::insert(
    const String& key,
    const void* value,
    size_t value_size) {
  insert(key.c_str(), key.length(), value, value_size);
}

void MDBTransaction::insert(
    const Buffer& key,
    const void* value,
    size_t value_size) {
  insert(key.data(), key.size(), value, value_size);
}

void MDBTransaction::insert(
    const void* key,
    size_t key_size,
    const void* value,
    size_t value_size) {
  MDB_val mkey, mval;
  mkey.mv_data = const_cast<void*>(key);
  mkey.mv_size = key_size;
  mval.mv_data = const_cast<void*>(value);
  mval.mv_size = value_size;

  auto flags = 0;
  if (!opts_->duplicate_keys) {
    flags |= MDB_NOOVERWRITE;
  }

  auto rc = mdb_put(mdb_txn_, mdb_handle_, &mkey, &mval, flags);
  if (rc != 0) {
    auto err = String(mdb_strerror(rc));
    RAISEF(kRuntimeError, "mdb_put() failed: $0", err);
  }
}

void MDBTransaction::update(const String& key, const String& value) {
  update(key.c_str(), key.length(), value.c_str(), value.length());
}

void MDBTransaction::update(const String& key, const Buffer& value) {
  update(key.c_str(), key.length(), value.data(), value.size());
}

void MDBTransaction::update(const Buffer& key, const Buffer& value) {
  update(key.data(), key.size(), value.data(), value.size());
}

void MDBTransaction::update(
    const String& key,
    const void* value,
    size_t value_size) {
  update(key.c_str(), key.length(), value, value_size);
}

void MDBTransaction::update(
    const Buffer& key,
    const void* value,
    size_t value_size) {
  update(key.data(), key.size(), value, value_size);
}

void MDBTransaction::update(
    const void* key,
    size_t key_size,
    const void* value,
    size_t value_size) {
  if (opts_->duplicate_keys) {
    auto cur = getCursor();

    if (cur->set(key, key_size)) {
      cur->put(key, key_size, value, value_size);
    } else {
      insert(key, key_size, value, value_size);
    }
  } else {
    MDB_val mkey, mval;
    mkey.mv_data = const_cast<void*>(key);
    mkey.mv_size = key_size;
    mval.mv_data = const_cast<void*>(value);
    mval.mv_size = value_size;

    auto rc = mdb_put(mdb_txn_, mdb_handle_, &mkey, &mval, 0);
    if (rc != 0) {
      auto err = String(mdb_strerror(rc));
      RAISEF(kRuntimeError, "mdb_put() failed: $0", err);
    }
  }
}

void MDBTransaction::del(const String& key) {
  del(key.c_str(), key.length());
}

void MDBTransaction::del(const Buffer& key) {
  del(key.data(), key.size());
}

void MDBTransaction::del(const void* key, size_t key_size) {
  MDB_val mkey;
  mkey.mv_data = const_cast<void*>(key);
  mkey.mv_size = key_size;

  auto rc = mdb_del(mdb_txn_, mdb_handle_, &mkey, NULL);
  if (rc != 0) {
    auto err = String(mdb_strerror(rc));
    RAISEF(kRuntimeError, "mdb_del() failed: $0", err);
  }
}


}
}
