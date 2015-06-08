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
#include "fnord-base/Language.h"
#include "fnord-base/random.h"
#include "fnord-mdb/MDB.h"
#include "fnord-msg/MessageSchema.h"
#include "ItemRef.h"
#include "logjoin/TrackedSession.h"
#include "logjoin/TrackedQuery.h"
#include "DocStore.h"
#include "IndexChangeRequest.h"
#include "DocIndex.h"
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
      msg::MessageSchemaRepository* schemas,
      bool dry_run);

  void setNormalize(
    Function<fnord::String (Language lang, const fnord::String& query)> normalizeCb);

  void setGetField(
    Function<Option<String> (const DocID& docid, const String& feature)> getFieldCb);

  Buffer trackedSessionToJoinedSession(TrackedSession& session);
  void onSession(
      mdb::MDBTransaction* txn,
      TrackedSession& session);

  size_t num_sessions;

protected:
  msg::MessageSchemaRepository* schemas_;
  Function<fnord::String (Language lang, const fnord::String& query)> normalize_;
  Function<Option<String> (const DocID& docid, const String& feature)> get_field_;
  bool dry_run_;
  Random rnd_;
  CurrencyConverter cconv_;
};
} // namespace cm

#endif
