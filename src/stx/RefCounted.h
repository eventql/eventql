/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef _STX_BASE_REFCOUNTED_H
#define _STX_BASE_REFCOUNTED_H

#include <atomic>
#include <stx/RefPtr.h>

namespace stx {

class RefCounted {
public:
  RefCounted();
  virtual ~RefCounted() {}

  void incRef();
  bool decRef();

protected:
  mutable std::atomic<unsigned> refcount_;
};

using AnyRef = RefPtr<RefCounted>;

} // namespace stx

#endif
