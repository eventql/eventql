// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/flow/vm/MatchClass.h>
#include <cassert>

namespace xzero {
namespace flow {
namespace vm {

std::string tos(MatchClass mc) {
  switch (mc) {
    case MatchClass::Same:
      return "Same";
    case MatchClass::Head:
      return "Head";
    case MatchClass::Tail:
      return "Tail";
    case MatchClass::RegExp:
      return "RegExp";
    default:
      assert(!"FIXME: NOT IMPLEMENTED");
      return "<FIXME>";
  }
}

}  // namespace vm
}  // namespace flow
}  // namespace xzero
