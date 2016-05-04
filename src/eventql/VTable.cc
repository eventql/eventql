/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <eventql/VTable.h>

using namespace stx;

namespace zbase {

//size_t VTableRDD::rowCount() const {
//  return rows_.size();
//}

void VTableRDD::forEach(
    Function<bool (const Vector<csql::SValue>&)> fn) {
  for (const auto& row : rows_) {
    if (!fn(row)) {
      return;
    }
  }
}

void VTableRDD::addRow(const Vector<csql::SValue>& row) {
  rows_.emplace_back(row);
}

RefPtr<VFSFile> VTableRDD::encode() const {
  RAISE(kNotYetImplementedError);
}

void VTableRDD::decode(RefPtr<VFSFile> src) {
  RAISE(kNotYetImplementedError);
}

} // namespace zbase
