/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_MDBUTIL_H
#define _STX_MDBUTIL_H
#include "stx/mdb/MDBTransaction.h"

namespace stx {
namespace mdb {

class MDBUtil {
public:

  template <typename T>
  static void increment(
      MDBTransaction* tx,
      const String& key,
      T value = 1);

  template <typename T>
  static Option<T> getAs(
      MDBTransaction* tx,
      const String& key);

};

}
}

#include "MDBUtil_impl.h"
#endif
