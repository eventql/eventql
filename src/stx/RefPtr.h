/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_BASE_REFPTR_H
#define _STX_BASE_REFPTR_H
#include <functional>
#include <memory>
#include <mutex>
#include <stdlib.h>
#include <atomic>
#include <stx/stdtypes.h>

namespace stx {

template <typename T>
class RefPtr {
public:
  using ValueType = T;

  RefPtr();
  RefPtr(std::nullptr_t);
  RefPtr(T* ref);

  RefPtr(const RefPtr<T>& other);
  RefPtr(RefPtr<T>&& other);

  ~RefPtr();
  RefPtr<T>& operator=(const RefPtr<T>& other);

  T& operator*() const;
  T* operator->() const;

  T* get() const;
  T* release();

  template<typename U>
  RefPtr<U> as() const;

  template <typename T_>
  RefPtr<T_> asInstanceOf() const;

protected:
  T* ref_;
};

template <typename T>
using AutoRef = RefPtr<T>;

template <typename T>
using RefPtrVector = Vector<RefPtr<T>>;

template <typename T>
RefPtr<T> mkRef(T* ptr);

template <typename T>
ScopedPtr<T> mkScoped(T* ptr);

} // namespace stx

#include "RefPtr_impl.h"
#endif
