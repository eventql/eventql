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
#include <eventql/DrilldownTree.h>

using namespace util;

namespace eventql {

DrilldownTree::DrilldownTree(
    size_t depth,
    size_t num_slots,
    size_t max_leaves) :
    depth_(depth),
    num_slots_(num_slots),
    max_leaves_(max_leaves),
    num_leaves_(0) {
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
        if (num_leaves_ == max_leaves_) {
          RAISE(kRuntimeError, "DrilldownTree cardinality is too large");
        }

        ++num_leaves_;
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

void DrilldownTree::toJSON(json::JSONOutputStream* json) {
  json->beginObject();
  toJSON(root_.get(), 0, json);
  json->endObject();
}

void DrilldownTree::toJSON(
    DrilldownTreeNode* node,
    size_t depth,
    json::JSONOutputStream* json) {
  if (depth == depth_) {
    auto leaf = static_cast<DrilldownTreeLeafNode*>(node);
    json->addObjectEntry("values");
    json->beginArray();

    for (size_t i = 0; i < num_slots_; ++i) {
      if (i > 0) {
        json->addComma();
      }

      json->addString(leaf->slots[i].getString());
    }

    json->endArray();
  } else {
    auto internal = static_cast<DrilldownTreeInternalNode*>(node);
    json->addObjectEntry("values");
    json->beginArray();

    auto begin = internal->slots.begin();
    auto end = internal->slots.end();
    for (auto cur = begin; cur != end; ++cur) {
      if (cur != begin) {
        json->addComma();
      }

      json->beginObject();
      json->addObjectEntry("key");
      json->addString(cur->first.getString());
      json->addComma();
      toJSON(cur->second.get(), depth + 1, json);
      json->endObject();
    }

    json->endArray();
  }
}

void DrilldownTree::walkLeafs(Function<void (DrilldownTreeLeafNode* leaf)> fn) {
  walkLeafs(root_.get(), 0, fn);
}

void DrilldownTree::walkLeafs(
    DrilldownTreeNode* node,
    size_t depth,
    Function<void (DrilldownTreeLeafNode* leaf)> fn) {
  if (depth == depth_) {
    auto leaf = static_cast<DrilldownTreeLeafNode*>(node);
    fn(leaf);
  } else {
    auto internal = static_cast<DrilldownTreeInternalNode*>(node);
    for (const auto& slot : internal->slots) {
      walkLeafs(slot.second.get(), depth + 1, fn);
    }
  }
}

DrilldownTreeLeafNode::DrilldownTreeLeafNode(
    size_t num_slots) :
    slots(num_slots) {}

}
