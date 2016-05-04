/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_MDBUTIL_IMPL_H
#define _STX_MDBUTIL_IMPL_H

namespace stx {
namespace mdb {

template <typename T>
void MDBUtil::increment(
    MDBTransaction* tx,
    const String& key,
    T value /* = 1 */) {
  auto old = tx->get(key);

  if (old.isEmpty()) {
    tx->insert(key, (void *) &value, sizeof(T));
  } else {
    if (old.get().size() < sizeof(T)) {
      RAISE(kRuntimeError, "old value is too small");
    }

    auto old_val = *((T*) (old.get().data()));
    auto new_val = old_val + value;

    tx->update(key, (void *) &new_val, sizeof(T));
  }
}

template <typename T>
Option<T> MDBUtil::getAs(
    MDBTransaction* tx,
    const String& key) {
  auto val = tx->get(key);

  if (val.isEmpty()) {
    return None<T>();
  } else {
    if (val.get().size() < sizeof(T)) {
      RAISE(kRuntimeError, "old value is too small");
    }

    return Some(*((T*) (val.get().data())));
  }
}



}
}
#endif
