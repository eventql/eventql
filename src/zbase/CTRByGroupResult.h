/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_GROUPBYCTRRESULT_H
#define _CM_GROUPBYCTRRESULT_H
#include <stx/stdtypes.h>
#include "zbase/GroupResult.h"

using namespace stx;

namespace cm {

template <typename GroupKeyType>
struct CTRByGroupResult : public GroupResult<GroupKeyType, CTRCounterData> {
  void toJSON(json::JSONOutputStream* json) const;
};

} // namespace cm

#include "CTRByGroupResult_impl.h"
#endif
