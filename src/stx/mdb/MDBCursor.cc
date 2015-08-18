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
#include "stx/mdb/MDBCursor.h"

namespace stx {
namespace mdb {

MDBCursor::MDBCursor(
    MDB_cursor* mdb_cur) :
    mdb_cur_(mdb_cur),
    is_closed_(false) {}

MDBCursor::~MDBCursor() {
  if (!is_closed_) {
    close();
  }
}

void MDBCursor::close() {
  if (is_closed_) {
    RAISE(kRuntimeError, "cursor was already closed");
  }

  is_closed_ = true;
  mdb_cursor_close(mdb_cur_);
}

bool MDBCursor::get(const String& key, Buffer* value) {
  void* val;
  size_t val_size;

  if (get(key.c_str(), key.length(), &val, &val_size)) {
    *value = Buffer(val, val_size);
    return true;
  } else {
    return false;
  }
}

bool MDBCursor::get(const Buffer& key, Buffer* value) {
  void* val;
  size_t val_size;

  if (get(key.data(), key.size(), &val, &val_size)) {
    *value = Buffer(val, val_size);
    return true;
  } else {
    return false;
  }
}

bool MDBCursor::get(
    const void* key,
    size_t key_size,
    void** value,
    size_t* value_size) {
  MDB_val mkey, mval;
  mkey.mv_data = const_cast<void*>(key);
  mkey.mv_size = key_size;

  auto rc = mdb_cursor_get(mdb_cur_, &mkey, &mval, MDB_SET);
  if (rc == MDB_NOTFOUND) {
    return false;
  }

  if (rc != 0) {
    auto err = String(mdb_strerror(rc));
    RAISEF(kRuntimeError, "mdb_cursor_get(FIRST) failed: $0", err);
  }

  *value = mval.mv_data;
  *value_size = mval.mv_size;
  return true;
}

bool MDBCursor::set(const void* key, size_t key_size) {
  MDB_val mkey;
  mkey.mv_data = const_cast<void*>(key);
  mkey.mv_size = key_size;

  auto rc = mdb_cursor_get(mdb_cur_, &mkey, nullptr, MDB_SET);
  if (rc == MDB_NOTFOUND) {
    return false;
  }

  if (rc != 0) {
    auto err = String(mdb_strerror(rc));
    RAISEF(kRuntimeError, "mdb_cursor_get(FIRST) failed: $0", err);
  }

  return true;
}

void MDBCursor::put(
    const void* key,
    size_t key_size,
    const void* value,
    size_t value_size) {
  MDB_val mkey, mval;
  mkey.mv_data = const_cast<void*>(key);
  mkey.mv_size = key_size;
  mval.mv_data = const_cast<void*>(value);
  mval.mv_size = value_size;

  auto rc = mdb_cursor_put(mdb_cur_, &mkey, &mval, MDB_CURRENT);

  if (rc != 0) {
    auto err = String(mdb_strerror(rc));
    RAISEF(kRuntimeError, "mdb_cursor_put() failed: $0", err);
  }
}

bool MDBCursor::getFirstOrGreater(Buffer* rkey, Buffer* rvalue) {
  void* key = rkey->data();
  size_t key_size = rkey->size();
  void* val;
  size_t val_size;

  if (getFirstOrGreater(&key, &key_size, &val, &val_size)) {
    *rkey = Buffer(key, key_size);
    *rvalue = Buffer(val, val_size);
    return true;
  } else {
    return false;
  }
}

bool MDBCursor::getFirstOrGreater(
    void** key,
    size_t* key_size,
    void** value,
    size_t* value_size) {
  MDB_val mkey, mval;
  mkey.mv_data = *key;
  mkey.mv_size = *key_size;

  auto rc = mdb_cursor_get(mdb_cur_, &mkey, &mval, MDB_SET_RANGE);
  if (rc == MDB_NOTFOUND) {
    return false;
  }

  if (rc != 0) {
    auto err = String(mdb_strerror(rc));
    RAISEF(kRuntimeError, "mdb_cursor_get(FIRST) failed: $0", err);
  }

  *key = mkey.mv_data;
  *key_size = mkey.mv_size;
  *value = mval.mv_data;
  *value_size = mval.mv_size;
  return true;
}

bool MDBCursor::getFirst(Buffer* rkey, Buffer* rvalue) {
  void* key;
  size_t key_size;
  void* val;
  size_t val_size;

  if (getFirst(&key, &key_size, &val, &val_size)) {
    *rkey = Buffer(key, key_size);
    *rvalue = Buffer(val, val_size);
    return true;
  } else {
    return false;
  }
}

bool MDBCursor::getFirst(
    void** key,
    size_t* key_size,
    void** value,
    size_t* value_size) {
  MDB_val mkey, mval;
  auto rc = mdb_cursor_get(mdb_cur_, &mkey, &mval, MDB_FIRST);
  if (rc == MDB_NOTFOUND) {
    return false;
  }

  if (rc != 0) {
    auto err = String(mdb_strerror(rc));
    RAISEF(kRuntimeError, "mdb_cursor_get(FIRST) failed: $0", err);
  }

  *key = mkey.mv_data;
  *key_size = mkey.mv_size;
  *value = mval.mv_data;
  *value_size = mval.mv_size;
  return true;
}

bool MDBCursor::getNext(Buffer* rkey, Buffer* rvalue) {
  void* key;
  size_t key_size;
  void* val;
  size_t val_size;

  if (getNext(&key, &key_size, &val, &val_size)) {
    *rkey = Buffer(key, key_size);
    *rvalue = Buffer(val, val_size);
    return true;
  } else {
    return false;
  }
}

bool MDBCursor::getNext(
    void** key,
    size_t* key_size,
    void** value,
    size_t* value_size) {
  MDB_val mkey, mval;
  auto rc = mdb_cursor_get(mdb_cur_, &mkey, &mval, MDB_NEXT);
  if (rc == MDB_NOTFOUND) {
    return false;
  }

  if (rc != 0) {
    auto err = String(mdb_strerror(rc));
    RAISEF(kRuntimeError, "mdb_cursor_get(NEXT) failed: $0", err);
  }

  *key = mkey.mv_data;
  *key_size = mkey.mv_size;
  *value = mval.mv_data;
  *value_size = mval.mv_size;
  return true;
}

bool MDBCursor::getPrev(Buffer* rkey, Buffer* rvalue) {
  void* key;
  size_t key_size;
  void* val;
  size_t val_size;

  if (getPrev(&key, &key_size, &val, &val_size)) {
    *rkey = Buffer(key, key_size);
    *rvalue = Buffer(val, val_size);
    return true;
  } else {
    return false;
  }
}

bool MDBCursor::getPrev(
    void** key,
    size_t* key_size,
    void** value,
    size_t* value_size) {
  MDB_val mkey, mval;
  auto rc = mdb_cursor_get(mdb_cur_, &mkey, &mval, MDB_PREV);
  if (rc == MDB_NOTFOUND) {
    return false;
  }

  if (rc != 0) {
    auto err = String(mdb_strerror(rc));
    RAISEF(kRuntimeError, "mdb_cursor_get(NEXT) failed: $0", err);
  }

  *key = mkey.mv_data;
  *key_size = mkey.mv_size;
  *value = mval.mv_data;
  *value_size = mval.mv_size;
  return true;
}

void MDBCursor::del() {
  mdb_cursor_del(mdb_cur_, 0);
}

}
}
