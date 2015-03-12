/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_STOPWORDDICTIONARY_H
#define _CM_STOPWORDDICTIONARY_H
#include "fnord-base/stdtypes.h"
#include "common.h"

using namespace fnord;

namespace cm {

class StopwordDictionary {
public:

  StopwordDictionary();

  const Set<String> stopwordsFor(Language lang) const;

protected:
};

} // namespace cm

#endif
