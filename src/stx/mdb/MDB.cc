/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "stx/mdb/MDB.h"

namespace stx {
namespace mdb {

RefPtr<MDB> MDB::open(
    const String& path,
    const MDBOptions& opts) {
  MDB_env* mdb_env;

  if (mdb_env_create(&mdb_env) != 0) {
    RAISE(kRuntimeError, "mdb_env_create() failed");
  }

  auto rc = mdb_env_set_mapsize(mdb_env, opts.maxsize);
  if (rc != 0) {
    auto err = String(mdb_strerror(rc));
    RAISEF(kRuntimeError, "mdb_set_mapsize() failed: $0", err);
  }

  int flags = 0;
  if (opts.readonly) {
    flags |= MDB_RDONLY;
  }

  if (!opts.sync) {
    flags |= MDB_NOSYNC;
  }

  RefPtr<MDB> mdb(
      new MDB(opts, mdb_env, path, opts.data_filename, opts.lock_filename));

  mdb->openDBHandle(flags, opts.duplicate_keys);
  return mdb;
}

MDB::MDB(
    const MDBOptions& opts,
    MDB_env* mdb_env,
    const String& path,
    const String& data_filename,
    const String& lock_filename) :
    opts_(opts),
    mdb_env_(mdb_env),
    path_(path),
    data_filename_("/" + data_filename),
    lock_filename_("/" + lock_filename) {}

MDB::~MDB() {
  mdb_env_close(mdb_env_);
}

void MDB::sync() {
  mdb_env_sync(mdb_env_, 1);
}

RefPtr<MDBTransaction> MDB::startTransaction(bool readonly /* = false */) {
  MDB_txn* txn;

  unsigned int flags = 0;
  if (readonly) {
    flags |= MDB_RDONLY;
  }

  auto rc = mdb_txn_begin(mdb_env_, NULL, flags, &txn);
  if (rc != 0) {
    auto err = String(mdb_strerror(rc));
    RAISEF(kRuntimeError, "mdb_txn_begin() failed: $0", err);
  }

  return RefPtr<MDBTransaction>(new MDBTransaction(txn, mdb_handle_, &opts_));
}

void MDB::openDBHandle(int flags, bool dupsort) {
  int rc = mdb_env_open(
      mdb_env_,
      path_.c_str(),
      flags,
      0664,
      data_filename_.c_str(),
      lock_filename_.c_str());

  if (rc != 0) {
    auto err = String(mdb_strerror(rc));
    RAISEF(kRuntimeError, "mdb_env_open($0) failed: $1", path_, err);
  }

  MDB_txn* txn;
  rc = mdb_txn_begin(mdb_env_, NULL, MDB_RDONLY, &txn);
  if (rc != 0) {
    auto err = String(mdb_strerror(rc));
    RAISEF(kRuntimeError, "mdb_txn_begin() failed: $0", err);
  }

  auto oflags = 0;
  if (dupsort) {
    oflags |= MDB_DUPSORT;
  }

  if (mdb_dbi_open(txn, NULL, oflags, &mdb_handle_) != 0) {
    RAISE(kRuntimeError, "mdb_dbi_open() failed");
  }

  rc = mdb_txn_commit(txn);
  if (rc != 0) {
    auto err = String(mdb_strerror(rc));
    RAISEF(kRuntimeError, "mdb_txn_commit() failed: $0", err);
  }
}

void MDB::setMaxSize(size_t size) {
  auto rc = mdb_env_set_mapsize(mdb_env_, size);
  if (rc != 0) {
    auto err = String(mdb_strerror(rc));
    RAISEF(kRuntimeError, "mdb_set_mapsize() failed: $0", err);
  }
}

void MDB::removeStaleReaders() {
  int dead;
  int rc = mdb_reader_check(mdb_env_, &dead);

  if (rc != 0) {
    auto err = String(mdb_strerror(rc));
    RAISEF(kRuntimeError, "mdb_reader_check() failed: $0", err);
  }
}

}
}
