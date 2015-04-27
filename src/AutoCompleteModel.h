/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_AUTOCOMPLETEMODEL_H
#define _CM_AUTOCOMPLETEMODEL_H
#include "fnord-json/json.h"
#include "analytics/TermInfo.h"

using namespace fnord;

namespace cm {

/**
 *  GET /autocomplete
 *
 */
class AutoCompleteModel : public RefCounted {
public:

  AutoCompleteModel(const String& filename) {}

};

}
#endif
