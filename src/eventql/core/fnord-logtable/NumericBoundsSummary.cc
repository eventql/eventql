/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
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
