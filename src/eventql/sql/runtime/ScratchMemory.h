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
#include <eventql/util/buffer.h>

#include "eventql/eventql.h"

namespace csql {

class ScratchMemory {
public:
  static const size_t kBlockSize;

  ScratchMemory();
  ScratchMemory(ScratchMemory&& other);
  ScratchMemory(const ScratchMemory& other) = delete;
  ScratchMemory& operator=(const ScratchMemory& other) = delete;
  ~ScratchMemory();

  void* alloc(size_t size);

  template <class ClassType, typename... ArgTypes>
  ClassType* construct(ArgTypes... args);

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

template <class ClassType, typename... ArgTypes>
ClassType* ScratchMemory::construct(ArgTypes... args) {
  auto mem = alloc(sizeof(ClassType));
  return new (mem) ClassType(args...);
}

}
