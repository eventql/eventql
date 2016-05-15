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
#include <eventql/sql/runtime/ScratchMemory.h>

#include "eventql/eventql.h"

namespace csql {

const size_t ScratchMemory::kBlockSize = 4096;

ScratchMemory::ScratchMemory() : head_(nullptr) {}

ScratchMemory::ScratchMemory(ScratchMemory&& other) : head_(other.head_) {
  other.head_ = nullptr;
}

ScratchMemory::~ScratchMemory() {
  for (auto cur = head_; cur != nullptr; ) {
    auto next = cur->next;
    free(cur);
    cur = next;
  }
}

void* ScratchMemory::alloc(size_t size) {
  if (!head_ || head_->used + size > head_->size) {
    appendBlock(std::max(size, kBlockSize));
  }

  auto off = head_->used;
  head_->used += size;
  return head_->data + off;
}

void ScratchMemory::appendBlock(size_t size) {
  auto block = (ScratchMemoryBlock*) malloc(sizeof(ScratchMemoryBlock) + size);
  block->size = size;
  block->used = 0;
  block->next = head_;
  head_ = block;
}

}
