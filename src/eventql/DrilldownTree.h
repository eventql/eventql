/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/util/json/jsonoutputstream.h>
#include <eventql/sql/svalue.h>

using namespace stx;

namespace zbase {

struct DrilldownTreeNode {
  virtual ~DrilldownTreeNode() {}
};

struct DrilldownTreeLeafNode : public DrilldownTreeNode {
  DrilldownTreeLeafNode(size_t num_slots);
  Vector<csql::SValue> slots;
};

struct DrilldownTreeInternalNode : public DrilldownTreeNode {
  HashMap<csql::SValue, ScopedPtr<DrilldownTreeNode>> slots;
};

class DrilldownTree : public RefCounted {
public:

  DrilldownTree(
      size_t depth,
      size_t num_slots,
      size_t max_leaves);

  DrilldownTreeLeafNode* lookupOrInsert(const csql::SValue* key);
  DrilldownTreeLeafNode* lookup(const csql::SValue* key);

  void walkLeafs(Function<void (DrilldownTreeLeafNode* leaf)> fn);

  void toJSON(json::JSONOutputStream* json);

protected:

  void toJSON(
      DrilldownTreeNode* node,
      size_t depth,
      json::JSONOutputStream* json);

  void walkLeafs(
      DrilldownTreeNode* node,
      size_t depth,
      Function<void (DrilldownTreeLeafNode* leaf)> fn);

  size_t depth_;
  size_t num_slots_;
  ScopedPtr<DrilldownTreeNode> root_;
  size_t max_leaves_;
  size_t num_leaves_;
};

}
