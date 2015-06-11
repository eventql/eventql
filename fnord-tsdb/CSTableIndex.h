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
#include <fnord-base/stdtypes.h>
#include <fnord-tsdb/TSDBTableScanSpec.pb.h>
#include <fnord-tsdb/TSDBClient.h>
#include <fnord-msg/MessageSchema.h>
#include <fnord-base/random.h>
#include <fnord-dproc/BlobRDD.h>

using namespace fnord;

namespace tsdb {

class CSTableIndex : public dproc::BlobRDD {
public:

  CSTableIndex(
      const TSDBTableScanSpec params,
      msg::MessageSchemaRepository* repo,
      tsdb::TSDBClient* tsdb);

  RefPtr<VFSFile> computeBlob(dproc::TaskContext* context) override;

  //Option<String> cacheKey() const override;

protected:
  TSDBTableScanSpec params_;
  tsdb::TSDBClient* tsdb_;
  RefPtr<msg::MessageSchema> schema_;
  Random rnd_;
};

}
