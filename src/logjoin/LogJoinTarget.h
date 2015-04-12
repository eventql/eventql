/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_LOGJOINTARGET_H
#define _CM_LOGJOINTARGET_H
#include "fnord-base/stdtypes.h"
#include "fnord-mdb/MDB.h"
#include "fnord-msg/MessageSchema.h"
#include "ItemRef.h"
#include "logjoin/TrackedSession.h"
#include "logjoin/TrackedQuery.h"

using namespace fnord;

namespace fnord {
namespace fts {
class Analyzer;
}
}

namespace cm {

class LogJoinTarget {
public:

  LogJoinTarget(
      const msg::MessageSchema& joined_sessions_schema,
      fts::Analyzer* analyzer);

  void onSession(
      mdb::MDBTransaction* txn,
      const TrackedSession& session);

  void onQuery(
      mdb::MDBTransaction* txn,
      const TrackedSession& session,
      const TrackedQuery& query);

  void onItemVisit(
      mdb::MDBTransaction* txn,
      const TrackedSession& session,
      const TrackedItemVisit& item_visit);

  void onItemVisit(
      mdb::MDBTransaction* txn,
      const TrackedSession& session,
      const TrackedItemVisit& item_visit,
      const TrackedQuery& query);

protected:
  msg::MessageSchema joined_sessions_schema_;
  fts::Analyzer* analyzer_;
};
} // namespace cm

#endif
