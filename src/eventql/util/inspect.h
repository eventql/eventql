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
#ifndef _libstx_UTIL_INSPECT_H_
#define _libstx_UTIL_INSPECT_H_
#include <set>
#include <string>
#include <vector>
#include "eventql/util/io/outputstream.h"

namespace stx {

template <typename T>
std::string inspect(const T& value);

template <typename T1, typename T2>
std::string inspect(const std::pair<T1, T2>& value);

template <typename T>
std::string inspect(const std::vector<T>& value);

template <typename T>
std::string inspect(const std::set<T>& value);

template <typename T>
std::string inspect(T* value);

template <typename H, typename... T>
std::vector<std::string> inspectAll(H head, T... tail);

template <typename H>
std::vector<std::string> inspectAll(H head);

template <typename... T>
void iputs(const char* fmt, T... values);

}

#include "inspect_impl.h"
#endif
