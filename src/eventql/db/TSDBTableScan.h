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
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/protobuf/MessageSchema.h>
#include <eventql/dproc/Task.h>
#include <eventql/db/TSDBTableScanSpec.pb.h>
#include <eventql/db/TSDBTableScanlet.h>
#include <eventql/db/TSDBClient.h>

#include "eventql/eventql.h"

namespace eventql {

template <typename ScanletType>
class TSDBTableScan : public dproc::RDD {
public:
  typedef typename ScanletType::ResultType ResultType;

  static ResultType mergeResults(
      dproc::TaskContext* context,
      RefPtr<ScanletType> scanlet);

  TSDBTableScan(
      const Buffer& params,
      RefPtr<ScanletType> scanlet,
      msg::MessageSchemaRepository* repo,
      TSDBClient* tsdb);

  TSDBTableScan(
      const TSDBTableScanSpec& params,
      RefPtr<ScanletType> scanlet,
      msg::MessageSchemaRepository* repo,
      TSDBClient* tsdb);

  void compute(dproc::TaskContext* context);
  List<dproc::TaskDependency> dependencies() const;

  RefPtr<VFSFile> encode() const override;
  void decode(RefPtr<VFSFile> data) override;

  Option<String> cacheKey() const override;

  ResultType* result();

protected:

  void onRow(const Buffer& row);

  void scanWithoutIndex(dproc::TaskContext* context);
  void scanWithCompactionWorker(dproc::TaskContext* context);

  TSDBTableScanSpec params_;
  RefPtr<ScanletType> scanlet_;
  RefPtr<msg::MessageSchema> schema_;
  TSDBClient* tsdb_;
};


} // namespace eventql

#include "TSDBTableScan_impl.h"
