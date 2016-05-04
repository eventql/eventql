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
#include <eventql/core/CompactionWorker.h>
#include <cstable/CSTableReader.h>
#include <cstable/RecordMaterializer.h>
#include <eventql/util/protobuf/MessageEncoder.h>

using namespace stx;

namespace zbase {

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

} // namespace zbase
