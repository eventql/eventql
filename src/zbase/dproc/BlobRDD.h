/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <zbase/dproc/Task.h>

using namespace stx;

namespace dproc {

class BlobRDD : public RDD {
public:

  BlobRDD();

  virtual RefPtr<VFSFile> computeBlob(TaskContext* context) = 0;

  void compute(TaskContext* context) override;
  RefPtr<VFSFile> encode() const override;
  void decode(RefPtr<VFSFile> data) override;

protected:
  RefPtr<VFSFile> blob_;
};

} // namespace dproc

