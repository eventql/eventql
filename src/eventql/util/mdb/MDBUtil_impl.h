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
#ifndef _STX_MDBUTIL_IMPL_H
#define _STX_MDBUTIL_IMPL_H

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
#endif
