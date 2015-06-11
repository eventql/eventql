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

using namespace fnord;

namespace tsdb {

template <typename ScanletType>
TSDBTableScanReducer<ScanletType>::TSDBTableScanReducer(
      const String& name,
      const TSDBTableScanSpec& params,
      RefPtr<ScanletType> scanlet,
      TSDBClient* tsdb) :
      name_(name),
      params_(params),
      scanlet_(scanlet),
      tsdb_(tsdb) {}

template <typename ScanletType>
void TSDBTableScanReducer<ScanletType>::compute(dproc::TaskContext* context) {
  for (int i = 0; i < context->numDependencies(); ++i) {
    auto shard = context->getDependencyAs<TSDBTableScanMapper<ScanletType>>(i);
    scanlet_->merge(*shard->result());
  }
}

template <typename ScanletType>
List<dproc::TaskDependency> TSDBTableScanReducer<ScanletType>::dependencies()
    const {
  auto stream_key = params_.stream_key();
  auto input_partitions = tsdb_->listPartitions(
      stream_key,
      params_.from(),
      params_.until());

  List<dproc::TaskDependency> deps;
  for (const auto& partition : input_partitions) {
    TSDBTableScanSpec dparams = params_;
    dparams.set_op(TSDB_OP_MAP);
    dparams.set_partition_key(partition);

    // FIXPAUL slow!!
    //auto pinfo = tsdb_->fetchPartitionInfo(stream_key, partition);
    //dparams.set_version(pinfo.version());

    deps.emplace_back(dproc::TaskDependency {
      .task_name = name_,
      .params = *msg::encode(dparams)
    });
  }

  return deps;
}

template <typename ScanletType>
RefPtr<VFSFile> TSDBTableScanReducer<ScanletType>::encode() const {
  return msg::encode(result_).get();
}

template <typename ScanletType>
void TSDBTableScanReducer<ScanletType>::decode(RefPtr<VFSFile> data) {
  msg::decode(data->data(), data->size(), &result_);
}

template <typename ScanletType>
typename TSDBTableScanReducer<ScanletType>::ResultType*
    TSDBTableScanReducer<ScanletType>::result() {
  return &result_;
}

} // namespace tsdb
