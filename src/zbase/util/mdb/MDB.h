/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_MDB_H
#define _STX_MDB_H
#include <memory>
#include <vector>
#include <liblmdb/lmdb.h>
#include "stx/logging.h"
#include "stx/autoref.h"
#include "stx/mdb/MDBTransaction.h"

namespace stx {
namespace mdb {

struct MDBOptions {
  MDBOptions() :
      readonly(false), // 1 GiB
      maxsize(1024 * 1024 * 1024),
      data_filename("data.mdb"),
      lock_filename("lock.mdb"),
      sync(true),
      duplicate_keys(true) {}

  bool readonly;
  size_t maxsize;
  String data_filename;
  String lock_filename;
  bool sync;
  bool duplicate_keys;
};

class MDB : public RefCounted {
public:

  static RefPtr<MDB> open(
      const String& path,
      const MDBOptions& opts = MDBOptions());

  MDB(const MDB& other) = delete;
  MDB& operator=(const MDB& other) = delete;
  ~MDB();

  RefPtr<MDBTransaction> startTransaction(bool readonly = false);

  void setMaxSize(size_t size);
  void sync();

  /**
   * Due to the way mdb transactions work (they may be left in a dangling state
   * when a program exits without closing mdb properly, e.g. due to a bug) this
   * method needs to be periodically called to clean up stale transactions.
   */
  void removeStaleReaders();

protected:

  MDB(
      const MDBOptions& opts,
      MDB_env* mdb_env,
      const String& path,
      const String& data_filename,
      const String& lock_filename);

  void openDBHandle(int flags, bool dupsort);

  MDBOptions opts_;
  MDB_env* mdb_env_;
  MDB_dbi mdb_handle_;

  const String path_;
  const String data_filename_;
  const String lock_filename_;
};

}
}
#endif
