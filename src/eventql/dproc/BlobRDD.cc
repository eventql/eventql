/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <eventql/dproc/BlobRDD.h>

using namespace stx;

namespace dproc {

BlobRDD::BlobRDD() : blob_(nullptr) {}

void BlobRDD::compute(TaskContext* context) {
  blob_ = computeBlob(context);
}

RefPtr<VFSFile> BlobRDD::encode() const {
  return blob_;
}

void BlobRDD::decode(RefPtr<VFSFile> data) {
  blob_ = data;
}

} // namespace dproc

