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
#include <utility>
#include <atomic>

namespace cortex {

template<typename T>
class CORTEX_API RefPtr {
 public:
  RefPtr();
  RefPtr(std::nullptr_t);
  RefPtr(T* obj);
  RefPtr(RefPtr<T>&& other);
  RefPtr(const RefPtr<T>& other);
  RefPtr<T>& operator=(RefPtr<T>&& other);
  RefPtr<T>& operator=(const RefPtr<T>& other);
  ~RefPtr();

  bool empty() const;
  size_t refCount() const;

  T* get() const;
  T* operator->() const;
  T& operator*() const;

  template<typename U>
  U* weak_as() const;

  template<typename U>
  RefPtr<U> as() const;

  T* release();
  void reset();

  bool operator==(const RefPtr<T>& other) const;
  bool operator!=(const RefPtr<T>& other) const;

  bool operator==(const T* other) const;
  bool operator!=(const T* other) const;

 private:
  T* obj_;
};

template<typename T, typename... Args>
CORTEX_API RefPtr<T> make_ref(Args... args);

// {{{ RefPtr impl
template<typename T>
inline RefPtr<T>::RefPtr()
    : obj_(nullptr) {
}

template<typename T>
inline RefPtr<T>::RefPtr(std::nullptr_t)
    : obj_(nullptr) {
}

template<typename T>
inline RefPtr<T>::RefPtr(T* obj)
    : obj_(obj) {
  if (obj_) {
    obj_->ref();
  }
}

template<typename T>
inline RefPtr<T>::RefPtr(const RefPtr<T>& other)
    : obj_(other.get()) {
  if (obj_) {
    obj_->ref();
  }
}

template<typename T>
inline RefPtr<T>::RefPtr(RefPtr<T>&& other)
    : obj_(other.release()) {
}

template<typename T>
inline RefPtr<T>& RefPtr<T>::operator=(RefPtr<T>&& other) {
  if (obj_)
    obj_->unref();

  obj_ = other.release();

  return *this;
}

template<typename T>
inline RefPtr<T>& RefPtr<T>::operator=(const RefPtr<T>& other) {
  if (obj_)
    obj_->unref();

  obj_ = other.obj_;

  if (obj_)
    obj_->ref();

  return *this;
}

template<typename T>
inline RefPtr<T>::~RefPtr() {
  if (obj_) {
    obj_->unref();
  }
}

template<typename T>
bool RefPtr<T>::empty() const {
  return obj_ == nullptr;
}

template<typename T>
size_t RefPtr<T>::refCount() const {
  if (obj_) {
    return obj_->refCount();
  } else {
    return 0;
  }
}

template<typename T>
inline T* RefPtr<T>::get() const {
  return obj_;
}

template<typename T>
inline T* RefPtr<T>::operator->() const {
  return obj_;
}

template<typename T>
inline T& RefPtr<T>::operator*() const {
  return *obj_;
}

template<typename T>
template<typename U>
inline U* RefPtr<T>::weak_as() const {
  return static_cast<U*>(obj_);
}

template<typename T>
template<typename U>
inline RefPtr<U> RefPtr<T>::as() const {
  return RefPtr<U>(static_cast<U*>(obj_));
}

template<typename T>
inline T* RefPtr<T>::release() {
  T* t = obj_;
  obj_ = nullptr;
  return t;
}

template<typename T>
inline void RefPtr<T>::reset() {
  if (obj_)
    obj_->unref();

  obj_ = nullptr;
}

template<typename T>
bool RefPtr<T>::operator==(const RefPtr<T>& other) const {
  return get() == other.get();
}

template<typename T>
bool RefPtr<T>::operator!=(const RefPtr<T>& other) const {
  return get() != other.get();
}

template<typename T>
bool RefPtr<T>::operator==(const T* other) const {
  return get() == other;
}

template<typename T>
bool RefPtr<T>::operator!=(const T* other) const {
  return get() != other;
}
//}}}
// {{{ free functions impl
template<typename T, typename... Args>
inline RefPtr<T> make_ref(Args... args) {
  return RefPtr<T>(new T(std::move(args)...));
}
// }}}

} // namespace cortex
