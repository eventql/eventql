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
#include "fnord-base/Currency.h"
#include "fnord-base/random.h"
#include "fnord-mdb/MDB.h"
#include "fnord-msg/MessageSchema.h"
#include "ItemRef.h"
#include "logjoin/TrackedSession.h"
#include "logjoin/TrackedQuery.h"
#include "DocStore.h"
#include "IndexChangeRequest.h"
#include "FeatureIndexWriter.h"
#include "ItemRef.h"

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
      fts::Analyzer* analyzer,
      RefPtr<FeatureIndexWriter> index,
      bool dry_run);

  void onSession(
      mdb::MDBTransaction* txn,
      TrackedSession& session);

  size_t num_sessions;

protected:
  msg::MessageSchema joined_sessions_schema_;
  fts::Analyzer* analyzer_;
  RefPtr<FeatureIndexWriter> index_;
  bool dry_run_;
  Random rnd_;
  CurrencyConverter cconv_;
};
} // namespace cm

#endif
