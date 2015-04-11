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

void CSTableBuilder::addRecord(const msg::MessageBuilder& msg) {
  for (const auto& f : schema_->fields) {
    addRecordField(0, 0, 0, msg, "", "", f);
  }

  abort();
}

void CSTableBuilder::addRecordField(
    uint32_t r,
    uint32_t rmax,
    uint32_t d,
    const msg::MessageBuilder& msg,
    const String& column,
    const String& path,
    const msg::MessageSchemaField& field) {
  auto this_r = r;

  auto num_reps = 1;
  if (field.optional && !msg.isSet(path + field.name)) {
    num_reps = 0;
  }

  if (field.repeated) {
    num_reps = msg.countRepetitions(path + field.name);
    ++rmax;
  }

  if (num_reps == 0) {
    writeNull(r, d, column + field.name, field);
    return;
  }

  if (field.optional || field.repeated) {
    ++d;
  }

  auto next_r = r;
  for (int i = 0; i < num_reps; ++i) {
    switch (field.type) {
      case msg::FieldType::OBJECT:
        for (const auto& f : field.fields) {
          addRecordField(
              next_r,
              rmax,
              d,
              msg,
              column + field.name + ".",
              field.repeated ?
                  StringUtil::format("$0$1[$2].", path, field.name, i) :
                  StringUtil::format("$0$1.", path, field.name),
              f);
        }
        break;

      default:
        writeField(next_r, d, msg, column + field.name, path + field.name, field);
        break;
    }

    next_r = rmax;
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
    const msg::MessageBuilder& msg,
    const String& column,
    const String& path,
    const msg::MessageSchemaField& field) {
  fnord::iputs("write field: $0 -- r=$1 d=$2 -- $3", column, r, d, path);
}



void CSTableBuilder::write(const String& filename) {
}


} // namespace cstable
} // namespace fnord

