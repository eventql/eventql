/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include <eventql/db/compaction_worker.h>
#include <eventql/io/cstable/cstable_reader.h>
#include <eventql/io/cstable/RecordMaterializer.h>
#include <eventql/util/protobuf/MessageEncoder.h>

#include "eventql/eventql.h"

namespace eventql {

template <typename ScanletType>
typename ScanletType::ResultType TSDBTableScan<ScanletType>::mergeResults(
    dproc::TaskContext* context,
    RefPtr<ScanletType> scanlet) {
  for (int i = 0; i < context->numDependencies(); ++i) {
    auto shard = context
        ->getDependency(i)
        ->getInstanceAs<TSDBTableScan<ScanletType>>();

    scanlet->merge(*shard->result());
  }

  return *scanlet->result();
}

template <typename ScanletType>
TSDBTableScan<ScanletType>::TSDBTableScan(
      const Buffer& params,
      RefPtr<ScanletType> scanlet,
      msg::MessageSchemaRepository* repo,
      TSDBClient* tsdb) :
      TSDBTableScan<ScanletType>(
          msg::decode<TSDBTableScanSpec>(params),
          scanlet,
          repo,
          tsdb) {}

template <typename ScanletType>
TSDBTableScan<ScanletType>::TSDBTableScan(
      const TSDBTableScanSpec& params,
      RefPtr<ScanletType> scanlet,
      msg::MessageSchemaRepository* repo,
      TSDBClient* tsdb) :
      params_(params),
      scanlet_(scanlet),
      schema_(repo->getSchema(params.schema_name())),
      tsdb_(tsdb) {}

template <typename ScanletType>
void TSDBTableScan<ScanletType>::compute(dproc::TaskContext* context) {
  //if (params_.use_cstable_index()) {
  //  scanWithCompactionWorker(context);
  //} else {
    scanWithoutIndex(context);
  //}
}

template <typename ScanletType>
void TSDBTableScan<ScanletType>::scanWithoutIndex(
    dproc::TaskContext* context) {
  tsdb_->fetchPartition(
      params_.tsdb_namespace(),
      params_.table_name(),
      SHA1Hash::fromHexString(params_.partition_key()),
      std::bind(
          &TSDBTableScan<ScanletType>::onRow,
          this,
          std::placeholders::_1));
}

template <typename ScanletType>
void TSDBTableScan<ScanletType>::scanWithCompactionWorker(
    dproc::TaskContext* context) {
  //auto dep = context->getDependency(0)->getInstanceAs<CompactionWorker>();
  //auto data = dep->encode();

  //cstable::CSTableReader reader(data);
  //cstable::RecordMaterializer materializer(
  //    schema_.get(),
  //    &reader,
  //    scanlet_->requiredFields());

  //auto rec_count = reader.numRecords();
  //// FIXPAUL soooo sloooooowww
  //for (size_t i = 0; i < rec_count; ++i) {
  //  msg::MessageObject robj;
  //  materializer.nextRecord(&robj);
  //  Buffer buf;
  //  msg::MessageEncoder::encode(robj, *schema_, &buf);
  //  onRow(buf);
  //}
}

template <typename ScanletType>
Option<String> TSDBTableScan<ScanletType>::cacheKey() const {
  auto ckey = scanlet_->cacheKey();

  if (ckey.isEmpty()) {
    return ckey;
  } else {
    return Some(
        StringUtil::format(
            "$0~$1~$2~$3~$4",
            ckey.get(),
            params_.tsdb_namespace(),
            params_.table_name(),
            params_.partition_key(),
            params_.version()));
  }
}

template <typename ScanletType>
void TSDBTableScan<ScanletType>::onRow(const Buffer& buffer) {
  typename ScanletType::RowType row;
  msg::decode(buffer, &row);
  scanlet_->scan(row);
}

template <typename ScanletType>
List<dproc::TaskDependency> TSDBTableScan<ScanletType>::dependencies()
    const {
  List<dproc::TaskDependency> deps;

  if (params_.use_cstable_index()) {
    auto dparams = params_;

    deps.emplace_back(dproc::TaskDependency {
      .task_name = "CompactionWorker",
      .params = *msg::encode(dparams)
    });
  }

  return deps;
}

template <typename ScanletType>
RefPtr<VFSFile> TSDBTableScan<ScanletType>::encode() const {
  return msg::encode(*scanlet_->result()).get();
}

template <typename ScanletType>
void TSDBTableScan<ScanletType>::decode(RefPtr<VFSFile> data) {
  msg::decode(data->data(), data->size(), scanlet_->result());
}

template <typename ScanletType>
typename TSDBTableScan<ScanletType>::ResultType*
    TSDBTableScan<ScanletType>::result() {
  return scanlet_->result();
}

//template <typename ScanletType>
//RefPtr<dproc::Task> TSDBTableScan<ScanletType>::mkTask(
//    const String& name,
//    const Buffer& params,
//    msg::MessageSchemaRepository* repo,
//    TSDBClient* tsdb) {
//  return mkTask(
//      name,
//      msg::decode<TSDBTableScanSpec>(params),
//      repo,
//      tsdb);
//}
//
//template <typename ScanletType>
//RefPtr<dproc::Task> TSDBTableScan<ScanletType>::mkTask(
//    const String& name,
//    const TSDBTableScanSpec& params,
//    msg::MessageSchemaRepository* repo,
//    TSDBClient* tsdb) {
//  auto scanlet_params = msg::decode<typename ScanletType::ParamType>(
//      params.scanlet_params().data(),
//      params.scanlet_params().size());
//
//  switch (params.op()) {
//
//    case TSDB_OP_MAP:
//      return new TSDBTableScan<ScanletType>(
//          name,
//          params,
//          new ScanletType(scanlet_params),
//          repo,
//          tsdb);
//
//    case TSDB_OP_MERGE:
//      return new TSDBTableScanReducer<ScanletType>(
//          name,
//          params,
//          new ScanletType(scanlet_params),
//          tsdb);
//
//  }
//}

} // namespace eventql
