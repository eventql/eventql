/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "stx/io/fileutil.h"
#include "stx/ieee754.h"
#include "stx/human.h"
#include <cstable/RecordShredder.h>

using namespace stx;

namespace cstable {

RecordShredder::RecordShredder(
    CSTableWriter* writer) :
    RecordShredder(writer, writer->schema()) {}

RecordShredder::RecordShredder(
    CSTableWriter* writer,
    const TableSchema* schema) :
    writer_(writer),
    schema_(schema) {}

void writeProtoNull(
    uint32_t r,
    uint32_t d,
    const String& column,
    TableSchema::Column* field,
    CSTableWriter* writer) {
  switch (field->type) {

    case ColumnType::SUBRECORD:
      for (const auto& f : field->subschema->columns()) {
        writeProtoNull(r, d, column + "." + f->name, f, writer);
      }

      break;

    default:
      auto col = writer->getColumnWriter(column);
      col->writeNull(r, d);
      break;

  }
}

void writeProtoField(
    uint32_t r,
    uint32_t d,
    const msg::MessageObject& msg,
    const String& column,
    TableSchema::Column* field,
    CSTableWriter* writer) {
  auto col = writer->getColumnWriter(column);

  switch (field->type) {

    case ColumnType::STRING: {
      auto& str = msg.asString();
      col->writeString(r, d, str.data(), str.size());
      break;
    }

    case ColumnType::UNSIGNED_INT: {
      col->writeUnsignedInt(r, d, msg.asUInt64());
      break;
    }

    case ColumnType::SIGNED_INT: {
      RAISE(kNotImplementedError);
    }

    case ColumnType::DATETIME: {
      col->writeDateTime(r, d, msg.asUnixTime());
      break;
    }

    case ColumnType::FLOAT: {
      col->writeFloat(r, d, msg.asDouble());
      break;
    }

    case ColumnType::BOOLEAN: {
      col->writeBoolean(r, d, msg.asBool());
      break;
    }

    case ColumnType::SUBRECORD:
      RAISE(kIllegalStateError);

  }
}

static void addProtoRecordField(
    uint32_t r,
    uint32_t rmax,
    uint32_t d,
    const msg::MessageObject& msg,
    const msg::MessageSchema& msg_schema,
    const String& column,
    TableSchema::Column* field,
    CSTableWriter* writer) {
  auto next_r = r;
  auto next_d = d;

  if (field->repeated) {
    ++rmax;
  }

  if (field->optional || field->repeated) {
    ++next_d;
  }

  size_t n = 0;
  auto field_id = msg_schema.fieldId(field->name);
  for (const auto& o : msg.asObject()) {
    if (o.id != field_id) { // FIXME
      continue;
    }

    ++n;

    switch (field->type) {
      case ColumnType::SUBRECORD: {
        auto o_schema = msg_schema.fieldSchema(field_id);
        for (const auto& f : field->subschema->columns()) {
          addProtoRecordField(
              next_r,
              rmax,
              next_d,
              o,
              *o_schema,
              column + field->name + ".",
              f,
              writer);
        }
        break;
      }

      default:
        writeProtoField(next_r, next_d, o, column + field->name, field, writer);
        break;
    }

    next_r = rmax;
  }

  if (n == 0) {
    if (!(field->optional || field->repeated)) {
      RAISEF(kIllegalArgumentError, "missing field: $0", column + field->name);
    }

    writeProtoNull(r, d, column + field->name, field, writer);
    return;
  }
}

void RecordShredder::addRecordFromProtobuf(const msg::DynamicMessage& msg) {
  addRecordFromProtobuf(msg.data(), *msg.schema());
}

void RecordShredder::addRecordFromProtobuf(
    const msg::MessageObject& msg,
    const msg::MessageSchema& schema) {
  for (const auto& f : schema_->columns()) {
    addProtoRecordField(0, 0, 0, msg, schema, "", f, writer_);
  }

  writer_->addRow();
}

void RecordShredder::addRecordsFromCSV(CSVInputStream* csv) {
  Vector<String> columns;
  csv->readNextRow(&columns);

  Set<String> missing_columns;
  for (const auto& col : writer_->columns()) {
    missing_columns.emplace(col.column_name);
  }

  Vector<RefPtr<ColumnWriter>> column_writers;
  Vector<ColumnType> field_types;
  for (const auto& col : columns) {
    if (!writer_->hasColumn(col)) {
      RAISEF(kRuntimeError, "column '$0' not found in schema", col);
    }

    auto cwriter = writer_->getColumnWriter(col);
    missing_columns.erase(col);
    column_writers.emplace_back(cwriter);
    field_types.emplace_back(cwriter->type());
  }

  Vector<RefPtr<ColumnWriter>> missing_column_writers;
  for (const auto& col : missing_columns) {
    auto cwriter = writer_->getColumnWriter(col);
    if (cwriter->maxDefinitionLevel() == 0) {
      RAISEF(kRuntimeError, "missing required column: $0", col);
    }

    missing_column_writers.emplace_back(cwriter);
  }

  Vector<String> row;
  while (csv->readNextRow(&row)) {
    for (size_t i = 0; i < row.size() && i < columns.size(); ++i) {
      const auto& col = column_writers[i];
      const auto& val = row[i];

      if (Human::isNullOrEmpty(val)) {
        if (col->maxDefinitionLevel() == 0) {
          RAISEF(
              kRuntimeError,
              "missing value for required column: $0",
              columns[i]);
        }

        col->writeNull(0, 0);
        continue;
      }

      switch (field_types[i]) {

        case ColumnType::STRING: {
          col->writeString(0, col->maxDefinitionLevel(), val.data(), val.size());
          break;
        }

        case ColumnType::UNSIGNED_INT: {
          uint64_t v;
          try {
            v = std::stoull(val);
          } catch (const StandardException& e) {
            RAISEF(kTypeError, "can't convert '$0' to UINT64", val);
          }

          col->writeUnsignedInt(0, col->maxDefinitionLevel(), v);
          break;
        }

        case ColumnType::SIGNED_INT: {
          int64_t v;
          try {
            v = std::stoll(val);
          } catch (const StandardException& e) {
            RAISEF(kTypeError, "can't convert '$0' to INT64", val);
          }

          col->writeSignedInt(0, col->maxDefinitionLevel(), v);
          break;
        }

        case ColumnType::DATETIME: {
          auto t = Human::parseTime(val);
          if (t.isEmpty()) {
            RAISEF(kTypeError, "can't convert '$0' to DATETIME", val);
          }

          col->writeDateTime(0, col->maxDefinitionLevel(), t.get());
          break;
        }

        case ColumnType::FLOAT: {
          double v;
          try {
            v = std::stod(val);
          } catch (const StandardException& e) {
            RAISEF(kTypeError, "can't convert '$0' to DOUBLE", val);
          }

          col->writeFloat(0, col->maxDefinitionLevel(), v);
          break;
        }

        case ColumnType::BOOLEAN: {
          auto b = Human::parseBoolean(val);
          auto v = !b.isEmpty() && b.get();
          col->writeBoolean(0, col->maxDefinitionLevel(), v);
          break;
        }

        case ColumnType::SUBRECORD:
          RAISE(kIllegalStateError, "can't read SUBRECORDs from CSV");

      }
    }

    for (auto& col : missing_column_writers) {
      col->writeNull(0, 0);
    }

    writer_->addRow();
  }
}

} // namespace cstable


