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
TSDBTableScanMapper<ScanletType>::TSDBTableScanMapper(
      const String& name,
      const TSDBTableScanSpec& params,
      RefPtr<ScanletType> scanlet,
      TSDBClient* tsdb) :
      params_(params),
      scanlet_(scanlet) {}

template <typename ScanletType>
void TSDBTableScanMapper<ScanletType>::compute(dproc::TaskContext* context) {
  tsdb_->fetchPartition(
      scanlet_->streamKey(),
      params_.partition_key(),
      std::bind(
          &TSDBTableScanMapper<ScanletType>::onRow,
          this,
          std::placeholders::_1));
}

template <typename ScanletType>
void TSDBTableScanMapper<ScanletType>::onRow(const Buffer& buffer) {
  typename ScanletType::RowType row;
  msg::decode(buffer, &row);
  scanlet_->scan(&row, &result_);
}

template <typename ScanletType>
List<dproc::TaskDependency> TSDBTableScanMapper<ScanletType>::dependencies()
    const {
  List<dproc::TaskDependency> deps;
  return deps;
}

template <typename ScanletType>
RefPtr<VFSFile> TSDBTableScanMapper<ScanletType>::encode() const {
  return msg::encode(result_).get();
}

template <typename ScanletType>
void TSDBTableScanMapper<ScanletType>::decode(RefPtr<VFSFile> data) {
  msg::decode(data->data(), data->size(), &result_);
}

template <typename ScanletType>
typename TSDBTableScanMapper<ScanletType>::ResultType*
    TSDBTableScanMapper<ScanletType>::result() {
  return &result_;
}

} // namespace tsdb
