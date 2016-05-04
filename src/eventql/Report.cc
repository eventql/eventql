/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "eventql/Report.h"

using namespace stx;

namespace zbase {

ReportRDD::ReportRDD(
    RefPtr<ReportSource> input,
    RefPtr<ReportSink> output) :
    input_(input),
    output_(output),
    result_(nullptr) {}

void ReportRDD::onInit() {}

void ReportRDD::onFinish() {}

void ReportRDD::compute(dproc::TaskContext* context) {
  output_->open();
  onInit();
  input_->read(context);
  onFinish();
  result_ = output_->finalize();
}

List<dproc::TaskDependency> ReportRDD::dependencies() const {
  return input_->dependencies();
}

RefPtr<ReportSource> ReportRDD::input() {
  return input_;
}

RefPtr<ReportSink> ReportRDD::output() {
  return output_;
}

Option<String> ReportRDD::cacheKey() const {
  return cache_key_;
}

void ReportRDD::setCacheKey(const String& key) {
  cache_key_ = Some(key);
}

RefPtr<VFSFile> ReportRDD::encode() const {
  return result_;
}

void ReportRDD::decode(RefPtr<VFSFile> data) {
  result_ = data;
}

String ReportRDD::contentType() const {
  return output_->contentType();
}

void rowToJSON(
    const Vector<String>& cols,
    const Vector<csql::SValue>& row,
    json::JSONOutputStream* json) {
  json->beginObject();

  for (int i = 0; i < std::min(cols.size(), row.size()); ++i) {
    if (i > 0) {
      json->addComma();
    }

    json->addObjectEntry(cols[i]);
    json->addString(row[i].getString());
  }

  json->endObject();
}

} // namespace zbase

