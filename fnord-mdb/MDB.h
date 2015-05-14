/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_MDB_H
#define _FNORD_MDB_H
#include <memory>
#include <vector>
#include <liblmdb/lmdb.h>
#include "fnord-base/logging.h"
#include "fnord-base/autoref.h"
#include "fnord-mdb/MDBTransaction.h"

namespace fnord {
namespace mdb {

class MDB : public RefCounted {
public:

  static RefPtr<MDB> open(
      const String& path,
      bool readonly = false,
      size_t maxsize = 1024 * 1024 * 1024, // 1 GiB
      const String& data_filename = "data.mdb",
      const String& lock_filename = "lock.mdb",
      bool sync = true);

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
      MDB_env* mdb_env,
      const String& path,
      const String& data_filename,
      const String& lock_filename);

  void openDBHandle(int flags);

  MDB_env* mdb_env_;
  MDB_dbi mdb_handle_;

  const String path_;
  const String data_filename_;
  const String lock_filename_;
};

}
}
#endif
