/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <fnord/stdtypes.h>
#include <fnord/buffer.h>

using namespace fnord;

namespace csql {

class ScratchMemory {
public:
  static const size_t kBlockSize;

  ScratchMemory();
  ~ScratchMemory();

  void* alloc(size_t size);

protected:
  struct ScratchMemoryBlock {
    size_t size;
    size_t used;
    ScratchMemoryBlock* next;
    char data[];
  };

  void appendBlock(size_t size);

  ScratchMemoryBlock* head_;
};

}
