/**
 *  This file is part of the "libstx" project
 *    (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
 *    (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
 *
 *  libstx is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU Affero General Public License v3.0.
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <vector>
#include <string>

namespace stx {

class StackTrace {
 public:
  StackTrace();
  StackTrace(StackTrace&&);
  StackTrace& operator=(StackTrace&&);
  StackTrace(const StackTrace&);
  StackTrace& operator=(const StackTrace&);
  ~StackTrace();

  std::vector<std::string> symbols() const;
  void debugPrint(int fd);

  static std::string demangleSymbol(const char* symbol);

 private:
  void** frames_;
  int frameCount_;
};

} // namespace stx
