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
#include "fnord-cstable/BitPackedIntColumnWriter.h"
#include "fnord-cstable/UInt32ColumnWriter.h"
#include "fnord-cstable/StringColumnWriter.h"
#include "fnord-cstable/BooleanColumnWriter.h"

namespace fnord {
namespace cstable {

CSTableBuilder::CSTableBuilder(
    msg::MessageSchema* schema) :
    schema_(schema) {
  for (const auto& f : schema_->fields) {
    createColumns("", 0, 0, f);
  }
}

void CSTableBuilder::createColumns(
    const String& prefix,
    uint32_t r_max,
    uint32_t d_max,
    const msg::MessageSchemaField& field) {
  auto colname = prefix + field.name;
  auto typesize = field.type_size;

  if (field.repeated) {
    ++r_max;
  }

  if (field.repeated || field.optional) {
    ++d_max;
  }

  switch (field.type) {
    case msg::FieldType::OBJECT:
      for (const auto& f : field.fields) {
        createColumns(colname + ".", r_max, d_max, f);
      }
      break;

    case msg::FieldType::UINT32:
      if (field.encoding == msg::EncodingHint::BITPACK) {
        columns_.emplace(
            colname,
            new cstable::BitPackedIntColumnWriter(r_max, d_max, typesize));
      } else {
        columns_.emplace(
            colname,
            new cstable::UInt32ColumnWriter(r_max, d_max));
      }
      break;

    case msg::FieldType::STRING:
      columns_.emplace(
          colname,
          new cstable::StringColumnWriter(r_max, d_max, typesize));
      break;

    case msg::FieldType::BOOLEAN:
      columns_.emplace(
          colname,
          new cstable::BooleanColumnWriter(r_max, d_max));
      break;

  }
}

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

  if (field.repeated) {
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
        writeField(next_r, next_d, o, column + field.name, field);
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
      auto col = columns_.find(column);
      col->second->addNull(r, d);
      break;

  }
}

void CSTableBuilder::writeField(
    uint32_t r,
    uint32_t d,
    const msg::MessageObject& msg,
    const String& column,
    const msg::MessageSchemaField& field) {
  auto col = columns_.find(column);

  switch (field.type) {

    case msg::FieldType::STRING: {
      auto& str = msg.asString();
      col->second->addDatum(r, d, str.data(), str.size());
      break;
    }

    case msg::FieldType::UINT32: {
      uint32_t val = msg.asUInt32();
      col->second->addDatum(r, d, &val, sizeof(val));
      break;
    }

    case msg::FieldType::BOOLEAN: {
      uint8_t val = msg.asBool() ? 1 : 0;
      col->second->addDatum(r, d, &val, sizeof(val));
      break;
    }

    case msg::FieldType::OBJECT:
      RAISE(kIllegalStateError);

  }
}



void CSTableBuilder::write(const String& filename) {
}


} // namespace cstable
} // namespace fnord

