/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_MDBUTIL_IMPL_H
#define _FNORD_MDBUTIL_IMPL_H

namespace fnord {
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

  fnord::iputs("incr $0 => $1", key, value);
}


}
}
#endif
