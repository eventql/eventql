// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <cortex-base/Api.h>
#include <atomic>

namespace cortex {

class CORTEX_API RefCounted {
 public:
  RefCounted();
  virtual ~RefCounted();

  void ref();
  bool unref();

  CORTEX_DEPRECATED void incRef() { ref(); }
  CORTEX_DEPRECATED void decRef() { unref(); }

  unsigned refCount() const;

 private:
  std::atomic<unsigned> refCount_;
};

// {{{ RefCounted impl
inline RefCounted::RefCounted()
    : refCount_(0) {
}

inline RefCounted::~RefCounted() {
}

inline unsigned RefCounted::refCount() const {
  return refCount_.load();
}

inline void RefCounted::ref() {
  refCount_++;
}

inline bool RefCounted::unref() {
  if (std::atomic_fetch_sub_explicit(&refCount_, 1u,
                                     std::memory_order_release) == 1) {
    std::atomic_thread_fence(std::memory_order_acquire);
    delete this;
    return true;
  }

  return false;
}
// }}}

} // namespace cortex
