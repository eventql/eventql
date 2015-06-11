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
#include <fnord-tsdb/CSTableIndex.h>
#include <cstable/CSTableReader.h>
#include <cstable/RecordMaterializer.h>
#include <fnord-msg/MessageEncoder.h>

using namespace fnord;

namespace tsdb {

template <typename ScanletType>
TSDBTableScanMapper<ScanletType>::TSDBTableScanMapper(
      const String& name,
      const TSDBTableScanSpec& params,
      RefPtr<ScanletType> scanlet,
      msg::MessageSchemaRepository* repo,
      TSDBClient* tsdb) :
      params_(params),
      scanlet_(scanlet),
      schema_(repo->getSchema(params.schema_name())),
      tsdb_(tsdb) {}

template <typename ScanletType>
void TSDBTableScanMapper<ScanletType>::compute(dproc::TaskContext* context) {
  if (params_.use_cstable_index()) {
    scanWithCSTableIndex(context);
  } else {
    scanWithoutIndex(context);
  }
}

template <typename ScanletType>
void TSDBTableScanMapper<ScanletType>::scanWithoutIndex(
    dproc::TaskContext* context) {
  tsdb_->fetchPartition(
      params_.stream_key(),
      params_.partition_key(),
      std::bind(
          &TSDBTableScanMapper<ScanletType>::onRow,
          this,
          std::placeholders::_1));
}

template <typename ScanletType>
void TSDBTableScanMapper<ScanletType>::scanWithCSTableIndex(
    dproc::TaskContext* context) {
  // FIXPAUL use getDepedencyResult
  auto dep = context->getDependency(0).asInstanceOf<CSTableIndex>();
  auto data = dep->encode();

  cstable::CSTableReader reader(data);
  cstable::RecordMaterializer materializer(
      schema_.get(),
      &reader,
      scanlet_->requiredFields());

  auto rec_count = reader.numRecords();
  // FIXPAUL soooo sloooooowww
  for (size_t i = 0; i < rec_count; ++i) {
    msg::MessageObject robj;
    materializer.nextRecord(&robj);
    Buffer buf;
    msg::MessageEncoder::encode(robj, *schema_, &buf);
    onRow(buf);
  }
}

template <typename ScanletType>
void TSDBTableScanMapper<ScanletType>::onRow(const Buffer& buffer) {
  typename ScanletType::RowType row;
  msg::decode(buffer, &row);
  scanlet_->scan(row);
}

template <typename ScanletType>
List<dproc::TaskDependency> TSDBTableScanMapper<ScanletType>::dependencies()
    const {
  List<dproc::TaskDependency> deps;

  if (params_.use_cstable_index()) {
    auto dparams = params_;
    dparams.set_op(TSDB_OP_BUILDINDEX);

    deps.emplace_back(dproc::TaskDependency {
      .task_name = "CSTableIndex",
      .params = *msg::encode(dparams)
    });
  }

  return deps;
}

template <typename ScanletType>
RefPtr<VFSFile> TSDBTableScanMapper<ScanletType>::encode() const {
  return msg::encode(*scanlet_->result()).get();
}

template <typename ScanletType>
void TSDBTableScanMapper<ScanletType>::decode(RefPtr<VFSFile> data) {
  msg::decode(data->data(), data->size(), scanlet_->result());
}

template <typename ScanletType>
typename TSDBTableScanMapper<ScanletType>::ResultType*
    TSDBTableScanMapper<ScanletType>::result() {
  return scanlet_->result();
}

} // namespace tsdb
