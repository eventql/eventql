/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/runtime/ScratchMemory.h>

using namespace stx;

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
