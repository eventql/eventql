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

namespace cm {

template <typename ProtoType>
TSDBTableScanSource<ProtoType>::TSDBTableScanSource(
    const tsdb::TSDBTableScanSpec& params,
    tsdb::TSDBService* tsdb) :
    params_(params),
    tsdb_(tsdb) {}

template <typename ProtoType>
void TSDBTableScanSource<ProtoType>::read(dproc::TaskContext* context) {
  //if (params_.use_cstable_index() && !required_fields_.empty()) {
  //  scanWithCSTableIndex(context);
  //} else {
    scanWithoutIndex(context);
  //}
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
  tsdb_->fetchPartitionWithSampling(
      params_.tsdb_namespace(),
      params_.table_name(),
      SHA1Hash::fromHexString(params_.partition_key()),
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
  //auto schema = msg::MessageSchema::fromProtobuf(ProtoType::descriptor());
  //auto dep = context->getDependency(0)->getInstanceAs<tsdb::CSTableIndex>();
  //auto data = dep->encode();

  //cstable::CSTableReader reader(data);
  //cstable::RecordMaterializer materializer(
  //    schema.get(),
  //    &reader,
  //    required_fields_);

  //auto rec_count = reader.numRecords();
  //// FIXPAUL soooo sloooooowww
  //for (size_t i = 0; i < rec_count; ++i) {
  //  if (i % 1000 == 0) {
  //    if (context->isCancelled()) {
  //      RAISE(kRuntimeError, "task cancelled");
  //    }
  //  }

  //  msg::MessageObject robj;
  //  materializer.nextRecord(&robj);
  //  Buffer buf;
  //  msg::MessageEncoder::encode(robj, *schema, &buf);
  //  fn_(msg::decode<ProtoType>(buf));
  //}
}

template <typename ProtoType>
List<dproc::TaskDependency> TSDBTableScanSource<ProtoType>::dependencies() const {
  List<dproc::TaskDependency> deps;

  if (params_.use_cstable_index() && !required_fields_.empty()) {
    auto dparams = params_;

    deps.emplace_back(dproc::TaskDependency {
      .task_name = "CSTableIndex",
      .params = *msg::encode(dparams)
    });
  }

  return deps;
}

template <typename ProtoType>
String TSDBTableScanSource<ProtoType>::cacheKey() const {
    return StringUtil::format(
        "tsdbtablescansource~$0~$1~$2~$3",
        params_.tsdb_namespace(),
        params_.table_name(),
        params_.partition_key(),
        params_.version());
}

} // namespace cm

