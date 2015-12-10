/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <zbase/DrilldownTree.h>

using namespace stx;

namespace zbase {

DrilldownTree::DrilldownTree(
    size_t depth,
    size_t num_slots) :
    depth_(depth),
    num_slots_(num_slots) {
  if (depth_ > 0) {
    root_ = mkScoped(new DrilldownTreeInternalNode());
  } else {
    root_ = mkScoped(new DrilldownTreeLeafNode(num_slots_));
  }
}

DrilldownTreeLeafNode* DrilldownTree::lookupOrInsert(
    const csql::SValue* key) {
  DrilldownTreeNode* cur = root_.get();
  for (size_t i = 0; i < depth_; ++i) {
    auto internal = static_cast<DrilldownTreeInternalNode*>(cur);
    auto& internal_slot = internal->slots[key[i]];
    cur = internal_slot.get();
    if (cur == nullptr) {
      if (i + 1 < depth_) {
        cur = new DrilldownTreeInternalNode();
      } else {
        cur = new DrilldownTreeLeafNode(num_slots_);
      }

      internal_slot.reset(cur);
    }
  }

  return static_cast<DrilldownTreeLeafNode*>(cur);
}

DrilldownTreeLeafNode* DrilldownTree::lookup(
    const csql::SValue* key) {
  DrilldownTreeNode* cur = root_.get();
  for (size_t i = 0; i < depth_; ++i) {
    auto internal = static_cast<DrilldownTreeInternalNode*>(cur);
    auto internal_slot = internal->slots.find(key[i]);
    if (internal_slot == internal->slots.end()) {
      return nullptr;
    } else {
      cur = internal_slot->second.get();
    }
  }

  return static_cast<DrilldownTreeLeafNode*>(cur);
}

DrilldownTreeLeafNode::DrilldownTreeLeafNode(
    size_t num_slots) :
    slots(num_slots) {}

}
