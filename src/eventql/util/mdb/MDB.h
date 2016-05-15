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
#ifndef _STX_MDB_H
#define _STX_MDB_H
#include <memory>
#include <vector>
#include <liblmdb/lmdb.h>
#include "eventql/util/logging.h"
#include "eventql/util/autoref.h"
#include "eventql/util/mdb/MDBTransaction.h"

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
