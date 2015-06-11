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
    const String& name,
    const Buffer& params,
    msg::MessageSchemaRepository* repo,
    TSDBClient* tsdb) {
  return mkTask(
      name,
      msg::decode<TSDBTableScanSpec>(params),
      repo,
      tsdb);
}

template <typename ScanletType>
RefPtr<dproc::Task> TSDBTableScan<ScanletType>::mkTask(
    const String& name,
    const TSDBTableScanSpec& params,
    msg::MessageSchemaRepository* repo,
    TSDBClient* tsdb) {
  auto scanlet_params = msg::decode<typename ScanletType::ParamType>(
      params.scanlet_params().data(),
      params.scanlet_params().size());

  switch (params.op()) {

    case TSDB_OP_MAP:
      return new TSDBTableScanMapper<ScanletType>(
          name,
          params,
          new ScanletType(scanlet_params),
          repo,
          tsdb);

    case TSDB_OP_MERGE:
      return new TSDBTableScanReducer<ScanletType>(
          name,
          params,
          new ScanletType(scanlet_params),
          tsdb);

  }
}


} // namespace tsdb
