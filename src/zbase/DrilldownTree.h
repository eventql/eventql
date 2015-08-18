/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/autoref.h>
#include <chartsql/svalue.h>
#include <chartsql/SFunction.h>

using namespace stx;

namespace cm {

struct DrilldownTreeNode : public RefCounted {
  HashMap<csql::SValue, RefPtr<DrilldownTreeNode>> groups;
};

struct DrilldownTree : public RefCounted {
  RefPtr<DrilldownTreeNode> root;
};

}
