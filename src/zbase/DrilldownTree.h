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
#include <csql/svalue.h>
#include <csql/SFunction.h>

using namespace stx;

namespace zbase {

struct DrilldownTreeNode {
  virtual ~DrilldownTreeNode() {}
};

struct DrilldownTreeLeafNode : public DrilldownTreeNode {
  Vector<csql::SValue> slots;
};

struct DrilldownTreeInternalNode : public DrilldownTreeNode {
  HashMap<csql::SValue, ScopedPtr<DrilldownTreeNode>> groups;
};

class DrilldownTree : public RefCounted {
public:

  DrilldownTree(size_t depth, size_t num_slots);

  DrilldownTreeLeafNode* lookup(
      const csql::SValue* key,
      bool insert = false);

protected:
  ScopedPtr<DrilldownTreeNode> root;
  size_t depth;
  size_t num_slots;
};

}
