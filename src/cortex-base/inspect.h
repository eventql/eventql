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
#include <set>
#include <string>
#include <vector>
#include <iosfwd>
//#include "cortex-base/io/outputstream.h"

namespace cortex {

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

// XXX this has nothing to do here. really needed?
template <typename... T>
void iputs(const char* fmt, T... values);

} // namespace cortex

#include "inspect-impl.h"
