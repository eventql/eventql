/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_LOGJOINUPLOAD_H
#define _CM_LOGJOINUPLOAD_H
#include "fnord-base/stdtypes.h"
#include "fnord-mdb/MDB.h"
#include "fnord-msg/MessageSchema.h"

using namespace fnord;

namespace cm {

class LogJoinUpload {
public:

  void upload(mdb::MDB* db);

protected:
};
} // namespace cm

#endif
