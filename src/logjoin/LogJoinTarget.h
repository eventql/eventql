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
#include "fnord/stdtypes.h"
#include "fnord/Currency.h"
#include "fnord/Language.h"
#include "fnord/random.h"
#include "fnord/mdb/MDB.h"
#include "fnord/protobuf/MessageSchema.h"
#include <inventory/ItemRef.h>
#include "logjoin/TrackedSession.h"
#include "logjoin/TrackedQuery.h"
#include "inventory/DocStore.h"
#include "inventory/IndexChangeRequest.h"
#include "inventory/DocIndex.h"
#include <inventory/ItemRef.h>

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

  Buffer joinSession(TrackedSession& session);

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
