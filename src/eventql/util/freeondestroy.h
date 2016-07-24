/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include <stdlib.h>

class FreeOnDestroy {
public:

  FreeOnDestroy() : ptr_(nullptr) {}
  FreeOnDestroy(void* ptr) : ptr_(ptr) {}
  FreeOnDestroy(const FreeOnDestroy& other) = delete;
  FreeOnDestroy& operator=(const FreeOnDestroy& other) = delete;
  ~FreeOnDestroy() { if(ptr_) { free(ptr_); } }

  void store(void* ptr) {
    if (ptr_) {
      free(ptr_);
    }

    ptr_ = ptr;
  }

  void release() {
    ptr_ = nullptr;
  }

  void* get() const {
    return ptr_;
  }

protected:
  void* ptr_;
};

class RunOnDestroy {
public:

  RunOnDestroy(std::function<void()> fn) : fn_(fn) {}
  RunOnDestroy(const FreeOnDestroy& other) = delete;
  RunOnDestroy& operator=(const FreeOnDestroy& other) = delete;
  ~RunOnDestroy() { fn_(); }

protected:
  std::function<void()> fn_;
};

