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
#include <unordered_map>
#include <memory>
#include <cassert>

namespace cortex {

struct CORTEX_API CustomData {
  CustomData(const CustomData&) = delete;
  CustomData& operator=(const CustomData&) = delete;
  CustomData() = default;
  virtual ~CustomData() {}
};

#define CUSTOMDATA_API_INLINE                                              \
 private:                                                                  \
  std::unordered_map<const void*, std::unique_ptr<cortex::CustomData>>      \
  customData_;                                                             \
 public:                                                                   \
  void clearCustomData() { customData_.clear(); }                          \
                                                                           \
  void clearCustomData(void* key) {                                        \
    auto i = customData_.find(key);                                        \
    if (i != customData_.end()) {                                          \
      customData_.erase(i);                                                \
    }                                                                      \
  }                                                                        \
                                                                           \
  cortex::CustomData* customData(const void* key) const {                   \
    auto i = customData_.find(key);                                        \
    return i != customData_.end() ? i->second.get() : nullptr;             \
  }                                                                        \
                                                                           \
  template <typename T>                                                    \
  T* customData(const void* key) const {                                   \
    auto i = customData_.find(key);                                        \
    return i != customData_.end() ? static_cast<T*>(i->second.get())       \
                                  : nullptr;                               \
  }                                                                        \
                                                                           \
  cortex::CustomData* setCustomData(const void* key,                            \
                                std::unique_ptr<cortex::CustomData>&& value) {  \
    auto res = value.get();                                                \
    customData_[key] = std::move(value);                                   \
    return res;                                                            \
  }                                                                        \
                                                                           \
  template <typename T, typename... Args>                                  \
  T* setCustomData(const void* key, Args&&... args) {                      \
    auto i = customData_.find(key);                                        \
    if (i != customData_.end()) return static_cast<T*>(i->second.get());   \
                                                                           \
    T* value = new T(std::forward<Args>(args)...);                         \
    customData_[key].reset(value);                                         \
    return value;                                                          \
  }

}  // namespace cortex
