/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-cstable/CSTableBuilder.h>

namespace fnord {
namespace cstable {

CSTableBuilder::CSTableBuilder(
    msg::MessageSchema* schema,
    HashMap<String, RefPtr<ColumnWriter>> column_writers) :
    schema_(schema),
    columns_(column_writers) {}

void CSTableBuilder::addRecord(const msg::MessageObject& msg) {
  for (const auto& f : schema_->fields) {
    addRecordField(0, 0, 0, msg, "", f);
  }

  abort();
}

void CSTableBuilder::addRecordField(
    uint32_t r,
    uint32_t rmax,
    uint32_t d,
    const msg::MessageObject& msg,
    const String& column,
    const msg::MessageSchemaField& field) {
  auto this_r = r;
  auto next_r = r;
  auto next_d = d;

  //auto num_reps = 1;
  //if (field.optional && !msg.isSet(path + field.name)) {
  //  num_reps = 0;
  //}

  if (field.repeated) {
    //num_reps = msg.countRepetitions(path + field.name);
    ++rmax;
  }

  if (field.optional || field.repeated) {
    ++next_d;
  }

  size_t n = 0;
  for (const auto& o : msg.asObject()) {
    if (o.id != field.id) {
      continue;
    }

    ++n;

    switch (field.type) {
      case msg::FieldType::OBJECT:
        for (const auto& f : field.fields) {
          addRecordField(
              next_r,
              rmax,
              next_d,
              o,
              column + field.name + ".",
              f);
        }
        break;

      default:
        writeField(next_r, next_d, msg, column + field.name, field);
        break;
    }

    next_r = rmax;
  }

  if (n == 0) {
    if (!(field.optional || field.repeated)) {
      RAISEF(kIllegalArgumentError, "missing field: $0", column + field.name);
    }

    writeNull(r, d, column + field.name, field);
    return;
  }
}

void CSTableBuilder::writeNull(
    uint32_t r,
    uint32_t d,
    const String& column,
    const msg::MessageSchemaField& field) {
  switch (field.type) {

    case msg::FieldType::OBJECT:
      for (const auto& f : field.fields) {
        writeNull(r, d, column + "." + f.name, f);
      }
      break;

    default:
      fnord::iputs("write null: $0 -- r=$1 d=$2", column, r, d);

  }
}

void CSTableBuilder::writeField(
    uint32_t r,
    uint32_t d,
    const msg::MessageObject& msg,
    const String& column,
    const msg::MessageSchemaField& field) {
  fnord::iputs("write field: $0 -- r=$1 d=$2", column, r, d);
}



void CSTableBuilder::write(const String& filename) {
}


} // namespace cstable
} // namespace fnord

