/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "zbase/TSDBTableScanSource.h"
#include <stx/fnv.h>
#include <zbase/core/CSTableIndex.h>
#include <cstable/CSTableReader.h>
#include <cstable/RecordMaterializer.h>
#include <stx/protobuf/MessageEncoder.h>

using namespace stx;

namespace zbase {

template <typename ProtoType>
TSDBTableScanSource<ProtoType>::TSDBTableScanSource(
    const zbase::TSDBTableScanSpec& params,
    zbase::PartitionMap* tsdb) :
    params_(params),
    tsdb_(tsdb) {}

template <typename ProtoType>
void TSDBTableScanSource<ProtoType>::read(dproc::TaskContext* context) {
  if (params_.use_cstable_index() && !required_fields_.empty()) {
    scanWithCSTableIndex(context);
  } else {
    scanWithoutIndex(context);
  }
}

template <typename ProtoType>
void TSDBTableScanSource<ProtoType>::forEach(
    Function <void (const ProtoType&)> fn) {
  fn_ = fn;
}

template <typename ProtoType>
void TSDBTableScanSource<ProtoType>::setRequiredFields(
    const Set<String>& fields) {
  required_fields_ = fields;
}

template <typename ProtoType>
void TSDBTableScanSource<ProtoType>::scanWithoutIndex(
    dproc::TaskContext* context) {
  auto partition = tsdb_->findPartition(
        params_.tsdb_namespace(),
        params_.table_name(),
        SHA1Hash::fromHexString(params_.partition_key()));

  if (partition.isEmpty()) {
    return;
  }

  auto preader = partition.get()->getReader();

  preader->fetchRecordsWithSampling(
      params_.sample_modulo(),
      params_.sample_index(),
      [this, context] (const Buffer& buf) {
    if (context->isCancelled()) {
      RAISE(kRuntimeError, "task cancelled");
    }

    fn_(msg::decode<ProtoType>(buf));
  });
}

template <typename ProtoType>
void TSDBTableScanSource<ProtoType>::scanWithCSTableIndex(
    dproc::TaskContext* context) {
  auto table = tsdb_->findTable(
        params_.tsdb_namespace(),
        params_.table_name());

  auto partition = tsdb_->findPartition(
        params_.tsdb_namespace(),
        params_.table_name(),
        SHA1Hash::fromHexString(params_.partition_key()));

  if (partition.isEmpty()) {
    return;
  }

  auto preader = partition.get()->getReader();
  auto cstable = preader->fetchCSTableFilename();
  if (cstable.isEmpty()) {
    return;
  }

  auto schema = table.get()->schema();

  cstable::CSTableReader reader(cstable.get());
  cstable::RecordMaterializer materializer(
      schema.get(),
      &reader,
      required_fields_);

  auto rec_count = reader.numRecords();
  // FIXPAUL soooo sloooooowww
  for (size_t i = 0; i < rec_count; ++i) {
    if (i % 1000 == 0) {
      if (context->isCancelled()) {
        RAISE(kRuntimeError, "task cancelled");
      }
    }

    msg::MessageObject robj;
    materializer.nextRecord(&robj);
    Buffer buf;
    msg::MessageEncoder::encode(robj, *schema, &buf);
    fn_(msg::decode<ProtoType>(buf));
  }
}

template <typename ProtoType>
List<dproc::TaskDependency> TSDBTableScanSource<ProtoType>::dependencies() const {
  List<dproc::TaskDependency> deps;
  return deps;
}

template <typename ProtoType>
String TSDBTableScanSource<ProtoType>::cacheKey() const {
  String version;

  auto partition = tsdb_->findPartition(
        params_.tsdb_namespace(),
        params_.table_name(),
        SHA1Hash::fromHexString(params_.partition_key()));

  if (!partition.isEmpty()) {
    auto preader = partition.get()->getReader();
    auto cstable_ver = preader->cstableVersion();
    if (!cstable_ver.isEmpty()) {
      version = cstable_ver.get().toString();
    }
  }

  return StringUtil::format(
      "tsdbtablescansource~$0~$1~$2~$3",
      params_.tsdb_namespace(),
      params_.table_name(),
      params_.partition_key(),
      version);
}

} // namespace zbase

