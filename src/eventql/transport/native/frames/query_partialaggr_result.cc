/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *   - Laura Schlimmer <laura@eventql.io>
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
#include "eventql/transport/native/frames/query_partialaggr_result.h"
#include "eventql/util/util/binarymessagewriter.h"

namespace eventql {
namespace native_transport {

QueryPartialAggrResultFrame::QueryPartialAggrResultFrame() :
    flags_(0),
    num_rows_(0) {}

void QueryPartialAggrResultFrame::setBody(std::string body) {
  body_ = body;
}

void QueryPartialAggrResultFrame::setBody(const char* data, size_t len) {
  body_ = std::string(data, len);
}

const std::string& QueryPartialAggrResultFrame::getBody() const {
  return body_;
}

std::string& QueryPartialAggrResultFrame::getBody() {
  return body_;
}

void QueryPartialAggrResultFrame::setNumRows(size_t num_rows) {
  num_rows_ = num_rows;
}

void QueryPartialAggrResultFrame::writeTo(OutputStream* os) {
  os->appendVarUInt(flags_);
  os->appendVarUInt(num_rows_);
  os->appendString(body_);
}


} // namespace native_transport
} // namespace eventql


