// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#pragma once

namespace cortex {

/* TypeIsReflected<T> */
template <typename T>
using TypeIsReflected = cortex::reflect::is_reflected<T>;

/* TypeIsVector<T> */
template <typename T, typename = void>
struct TypeIsVector {
  static const bool value = false;
};

template <typename T>
struct TypeIsVector<
    T,
    typename std::enable_if<
        std::is_same<
            T,
            std::vector<
                typename T::value_type,
                typename T::allocator_type>>::value>::type>  {
  static const bool value = true;
};

} // namespace cortex
