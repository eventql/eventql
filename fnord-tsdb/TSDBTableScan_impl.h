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
RefPtr<dproc::Task> TSDBTableScan<ScanletType>::mkTask(
    const DateTime& from,
    const DateTime& until,
    const Buffer& params) {
  return mkTask(
      from,
      until,
      msg::decode<TSDBTableScanSpec>(params));
}

template <typename ScanletType>
RefPtr<dproc::Task> TSDBTableScan<ScanletType>::mkTask(
    const DateTime& from,
    const DateTime& until,
    const TSDBTableScanSpec& params) {
  auto scanlet_params = msg::decode<typename ScanletType::ParamType>(
      params.scanlet_params().data(),
      params.scanlet_params().size());

  switch (params.op()) {

    case TSDB_OP_MERGE:
      return new TSDBTableScanReducer<ScanletType>(
          params,
          new ScanletType(scanlet_params));

  }
}


} // namespace tsdb
