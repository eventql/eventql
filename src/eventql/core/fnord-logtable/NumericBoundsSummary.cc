/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
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
#include <fnord-logtable/NumericBoundsSummary.h>
#include <fnord-logtable/TableChunkSummaryWriter.h>

namespace stx {
namespace logtable {

void NumericBoundsSummary::encode(util::BinaryMessageWriter* writer) const {
  writer->appendVarUInt(min_value);
  writer->appendVarUInt(max_value);
}

void NumericBoundsSummary::decode(util::BinaryMessageReader* reader) {
  min_value = reader->readVarUInt();
  max_value = reader->readVarUInt();
}

NumericBoundsSummaryBuilder::NumericBoundsSummaryBuilder(
    const String& summary_name,
    uint32_t field_id) :
    summary_name_(summary_name),
    field_id_(field_id),
    n_(0) {
  summary_.min_value = std::numeric_limits<double>::min();
  summary_.max_value = std::numeric_limits<double>::max();
}

void NumericBoundsSummaryBuilder::addRecord(const msg::MessageObject& record) {
  if (record.id == field_id_) {
    double val = getValue(record);

    if (n_ == 0 || val > summary_.max_value) {
      summary_.max_value = val;
    }

    if (n_ == 0 || val < summary_.min_value) {
      summary_.min_value = val;
    }

    ++n_;
    return;
  }

  if (record.type == msg::FieldType::OBJECT) {
    for (const auto& f : record.asObject()) {
      addRecord(f);
    }
  }
}

double NumericBoundsSummaryBuilder::getValue(
    const msg::MessageObject& record) const {
  switch (record.type) {
    case msg::FieldType::OBJECT:
    case msg::FieldType::STRING:
    case msg::FieldType::BOOLEAN:
      RAISE(kTypeError);

    case msg::FieldType::UINT32:
      return record.asUInt32();

  }
}

void NumericBoundsSummaryBuilder::commit(TableChunkSummaryWriter* writer) {
  util::BinaryMessageWriter buf;
  summary_.encode(&buf);
  writer->addSummary(summary_name_, buf.data(), buf.size());
}

}
}
