/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include <eventql/util/ieee754.h>
#include <eventql/io/cstable/RecordMaterializer.h>

#include "eventql/eventql.h"

namespace cstable {

RecordMaterializer::RecordMaterializer(
    msg::MessageSchema* schema,
    CSTableReader* reader,
    Set<String> columns /* = Set<String> {} */) {
  for (const auto& f : schema->fields()) {
    createColumns(
        "",
        0,
        Vector<Tuple<uint64_t, bool, uint32_t>>{},
        f,
        reader,
        columns);
  }
}

void RecordMaterializer::nextRecord(msg::MessageObject* record) {
  for (auto& col : columns_) {
    loadColumn(col.first, &col.second, record);
  }
}

void RecordMaterializer::skipRecord() {
  for (auto& c : columns_) {
    auto& column = c.second;

    for (;;) {
      column.fetchIfNotPending();
      column.consume();

      if (column.reader->nextRepetitionLevel() == 0) {
        break;
      }

      column.fetchIfNotPending();
      if (column.r == 0) {
        break;
      }
    }
  }
}

void RecordMaterializer::loadColumn(
    const String& column_name,
    ColumnState* column,
    msg::MessageObject* record) {
  Vector<size_t> indexes(column->reader->maxRepetitionLevel());

  for (;;) {
    column->fetchIfNotPending();

    if (column->r > 0) {
      ++indexes[column->r - 1];

      for (int x = column->r; x < indexes.size(); ++x) {
        indexes[x] = 0;
      }
    }

    if (column->defined) {
      insertValue(column, column->parents, indexes, record);
    } else {
      insertNull(column, column->parents, indexes, record);
    }

    column->consume();

    if (column->reader->nextRepetitionLevel() == 0) {
      break;
    }

    column->fetchIfNotPending();
    if (column->r == 0) {
      return;
    }
  }
}

void RecordMaterializer::createColumns(
    const String& prefix,
    uint32_t dmax,
    Vector<Tuple<uint64_t, bool, uint32_t>> parents,
    const msg::MessageSchemaField& field,
    CSTableReader* reader,
    const Set<String>& columns) {
  auto colname = prefix + field.name;

  if (field.repeated || field.optional) {
    ++dmax;
  }

  switch (field.type) {
    case msg::FieldType::OBJECT: {
      parents.emplace_back(field.id, field.repeated, dmax);

      for (const auto& f : field.schema->fields()) {
        createColumns(colname + ".", dmax, parents, f, reader, columns);
      }
      break;
    }

    default: {
      if (reader->hasColumn(colname) &&
          (columns.empty() || columns.count(colname) > 0)) {
        ColumnState colstate(reader->getColumnReader(colname));
        colstate.parents = parents;
        colstate.field_id = field.id;
        colstate.field_type = field.type;
        colstate.col_type = colstate.reader->type();
        columns_.emplace(colname, colstate);
      }
      break;
    }

  }
}

void RecordMaterializer::insertValue(
    ColumnState* column,
    Vector<Tuple<uint64_t, bool, uint32_t>> parents,
    Vector<size_t> indexes,
    msg::MessageObject* record) {
  if (parents.size() > 0) {
    auto parent = parents[0];
    parents.erase(parents.begin());

    // repeated...
    if (std::get<1>(parent)) {
      auto target_idx = indexes[0];
      size_t this_index = 0;
      indexes.erase(indexes.begin());

      for (auto& cld : record->asObject()) {
        if (cld.id == std::get<0>(parent)) {
          if (this_index == target_idx) {
            return insertValue(column, parents, indexes, &cld);
          }

          ++this_index;
        }
      }

      for (; this_index < target_idx; ++this_index) {
        record->addChild(std::get<0>(parent));
      }

      auto& new_cld = record->addChild(std::get<0>(parent));
      return insertValue(column, parents, indexes, &new_cld);

    // non repeated
    } else {
      for (auto& cld : record->asObject()) {
        if (cld.id == std::get<0>(parent)) {
          return insertValue(column, parents, indexes, &cld);
        }
      }

      auto& new_cld = record->addChild(std::get<0>(parent));
      return insertValue(column, parents, indexes, &new_cld);
    }
  }

  switch (column->field_type) {
    case msg::FieldType::OBJECT:
      break;

    case msg::FieldType::UINT32:
      record->addChild(
          column->field_id,
          (uint32_t) column->getUnsignedInteger());
      break;

    case msg::FieldType::UINT64:
      record->addChild(
          column->field_id,
          (uint64_t) column->getUnsignedInteger());
      break;

    case msg::FieldType::DATETIME:
      record->addChild(
          column->field_id,
          UnixTime(column->getUnsignedInteger()));
      break;

    case msg::FieldType::STRING:
      record->addChild(column->field_id, column->getString());
      break;

    case msg::FieldType::BOOLEAN:
      if (column->getUnsignedInteger() == 1) {
        record->addChild(column->field_id, msg::TRUE);
      } else {
        record->addChild(column->field_id, msg::FALSE);
      }
      break;

    case msg::FieldType::DOUBLE:
      record->addChild(column->field_id, column->getFloat());
      break;

  }
}

void RecordMaterializer::insertNull(
    ColumnState* column,
    Vector<Tuple<uint64_t, bool, uint32_t>> parents,
    Vector<size_t> indexes,
    msg::MessageObject* record) {
  if (parents.size() > 0) {
    auto parent = parents[0];
    parents.erase(parents.begin());

    // definition level
    if (std::get<2>(parent) > column->d) {
      return;
    }

    // repeated...
    if (std::get<1>(parent)) {
      auto target_idx = indexes[0];
      size_t this_index = 0;
      indexes.erase(indexes.begin());

      for (auto& cld : record->asObject()) {
        if (cld.id == std::get<0>(parent)) {
          if (this_index == target_idx) {
            return insertNull(column, parents, indexes, &cld);
          }

          ++this_index;
        }
      }

      for (; this_index < target_idx; ++this_index) {
        record->addChild(std::get<0>(parent));
      }

      auto& new_cld = record->addChild(std::get<0>(parent));
      return insertNull(column, parents, indexes, &new_cld);

    // non repeated
    } else {
      for (auto& cld : record->asObject()) {
        if (cld.id == std::get<0>(parent)) {
          return insertNull(column, parents, indexes, &cld);
        }
      }

      auto& new_cld = record->addChild(std::get<0>(parent));
      return insertNull(column, parents, indexes, &new_cld);
    }
  }
}

void RecordMaterializer::ColumnState::fetchIfNotPending() {
  if (pending) {
    return;
  }

  switch (col_type) {
    case ColumnType::SUBRECORD:
      RAISE(kIllegalStateError);
    case ColumnType::BOOLEAN:
      defined = reader->readUnsignedInt(&r, &d, &val_uint);
      break;
    case ColumnType::UNSIGNED_INT:
      defined = reader->readUnsignedInt(&r, &d, &val_uint);
      break;
    case ColumnType::SIGNED_INT:
      defined = reader->readSignedInt(&r, &d, &val_sint);
      break;
    case ColumnType::STRING:
      defined = reader->readString(&r, &d, &val_str);
      break;
    case ColumnType::FLOAT:
      defined = reader->readFloat(&r, &d, &val_float);
      break;
    case ColumnType::DATETIME:
      defined = reader->readUnsignedInt(&r, &d, &val_uint);
      break;
  };

  pending = true;
}

void RecordMaterializer::ColumnState::consume() {
  pending = false;
}

uint64_t RecordMaterializer::ColumnState::getUnsignedInteger() const {
  switch (col_type) {
    case ColumnType::SUBRECORD:
      RAISE(kIllegalStateError);
    case ColumnType::BOOLEAN:
    case ColumnType::UNSIGNED_INT:
    case ColumnType::DATETIME:
      return val_uint;
    case ColumnType::SIGNED_INT:
      return val_sint;
    case ColumnType::STRING:
      try {
        return std::stoull(val_str);
      } catch (...) {
        RAISEF(kIllegalArgumentError, "can't convert '$0' to uint", val_str);
      }
    case ColumnType::FLOAT:
      return val_float;
  };
}

int64_t RecordMaterializer::ColumnState::getSignedInteger() const {
  switch (col_type) {
    case ColumnType::SUBRECORD:
      RAISE(kIllegalStateError);
    case ColumnType::BOOLEAN:
    case ColumnType::UNSIGNED_INT:
    case ColumnType::DATETIME:
      return val_uint;
    case ColumnType::SIGNED_INT:
      return val_sint;
    case ColumnType::STRING:
      return std::stoll(val_str);
    case ColumnType::FLOAT:
      return val_float;
  };
}

String RecordMaterializer::ColumnState::getString() const {
  switch (col_type) {
    case ColumnType::SUBRECORD:
      RAISE(kIllegalStateError);
    case ColumnType::BOOLEAN:
    case ColumnType::UNSIGNED_INT:
    case ColumnType::DATETIME:
      return StringUtil::toString(val_uint);
    case ColumnType::SIGNED_INT:
      return StringUtil::toString(val_sint);
    case ColumnType::STRING:
      return val_str;
    case ColumnType::FLOAT:
      return StringUtil::toString(val_float);
  };
}

double RecordMaterializer::ColumnState::getFloat() const {
  switch (col_type) {
    case ColumnType::SUBRECORD:
      RAISE(kIllegalStateError);
    case ColumnType::BOOLEAN:
    case ColumnType::UNSIGNED_INT:
    case ColumnType::DATETIME:
      return val_uint;
    case ColumnType::SIGNED_INT:
      return val_sint;
    case ColumnType::STRING:
      return std::stod(val_str);
    case ColumnType::FLOAT:
      return val_float;
  };
}

} // namespace cstable


