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
#include <cortex-base/RuntimeError.h>

#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <memory>

namespace cortex {

struct None {};

/**
 * Option<> type class.
 */
template <typename T>
class CORTEX_API Option {
 public:
  Option() : value_(), valid_(false) {}
  Option(const None&) : value_(), valid_(false) {}
  Option(const T& value) : value_(value), valid_(true) {}
  Option(T&& value) : value_(std::move(value)), valid_(true) {}

  Option(const Option<T>& other) = default;
  Option<T>& operator=(const Option<T>& other) = default;

  Option<T>& operator=(Option<T>&& other) {
    value_ = std::move(other.value_);
    valid_ = other.valid_;
    other.valid_ = false;
    return *this;
  }

  bool isSome() const { return valid_; }
  bool isNone() const noexcept { return !valid_; }
  operator bool() const { return isSome(); }

  T& get() {
    require();
    return value_;
  }

  const T& get() const {
    require();
    return value_;
  }

  T& operator*() {
    require();
    return value_;
  }

  const T& operator*() const {
    require();
    return value_;
  }

  T* operator->() {
    require();
    return &value_;
  }

  const T* operator->() const {
    require();
    return &value_;
  }

  void require() const {
    if (isNone())
      RAISE(OptionUncheckedAccessToInstance);
  }

  void requireNone() const {
    if (!isNone())
      RAISE(OptionUncheckedAccessToInstance);
  }

  void clear() {
    value_ = T();
    valid_ = false;
  }

  template <typename U>
  Option<T> onSome(U block) const {
    if (isSome())
      block(get());

    return *this;
  }

  template <typename U>
  Option<T> onNone(U block) const {
    if (isNone())
      block();

    return *this;
  }

 private:
  T value_;
  bool valid_;
};

template <typename T>
inline bool operator==(const Option<T>& a, const Option<T>& b) {
  if (a.isSome() && b.isSome())
    return a.get() == b.get();

  if (a.isNone() && b.isNone())
    return true;

  return false;
}

template <typename T>
inline bool operator==(const Option<T>& a, const None& b) {
  return a.isNone();
}

template <typename T>
inline bool operator!=(const Option<T>& a, const Option<T>& b) {
  return !(a == b);
}

template <typename T>
inline bool operator!=(const Option<T>& a, const None& b) {
  return !(a == b);
}

template <typename T>
inline Option<T> Some(const T& value) {
  return Option<T>(value);
}

}  // namespace cortex
