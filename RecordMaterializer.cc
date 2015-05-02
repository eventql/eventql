/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-cstable/RecordMaterializer.h>

namespace fnord {
namespace cstable {

RecordMaterializer::RecordMaterializer(
    RefPtr<msg::MessageSchema> schema,
    CSTableReader* reader) :
    schema_(schema) {
  for (const auto& f : schema_->fields) {
    createColumns("", Vector<Pair<uint64_t, bool>>{}, f, reader);
  }
}

void RecordMaterializer::nextRecord(msg::MessageObject* record) {
  for (auto& col : columns_) {
    loadColumn(col.first, &col.second, record);
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
      insertNull(column, indexes, record);
    }

    column->consume();

    if (column->reader->eofReached()) {
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
    Vector<Pair<uint64_t, bool>> parents,
    const msg::MessageSchemaField& field,
    CSTableReader* reader) {
  auto colname = prefix + field.name;

  switch (field.type) {
    case msg::FieldType::OBJECT: {
      parents.emplace_back(field.id, field.repeated);

      for (const auto& f : field.fields) {
        createColumns(colname + ".", parents, f, reader);
      }
      break;
    }

    default: {
      ColumnState colstate(reader->getColumnReader(colname));
      colstate.parents = parents;
      colstate.field_id = field.id;
      colstate.field_type = field.type;
      columns_.emplace(colname, colstate);
      break;
    }

  }
}

void RecordMaterializer::insertValue(
    ColumnState* column,
    Vector<Pair<uint64_t, bool>> parents,
    Vector<size_t>& indexes,
    msg::MessageObject* record) {
  if (parents.size() > 0) {
    auto parent = parents[0];
    parents.erase(parents.begin());

    // repeated...
    if (parent.second) {
      auto target_idx = indexes[0];
      size_t this_index = 0;
      indexes.erase(indexes.begin());

      for (auto& cld : record->asObject()) {
        if (cld.id == parent.first) {
          if (this_index == target_idx) {
            return insertValue(column, parents, indexes, &cld);
          }

          ++this_index;
        }
      }

      for (; this_index < target_idx; ++this_index) {
        record->addChild(parent.first);
      }

      auto new_cld = record->addChild(parent.first);
      return insertValue(column, parents, indexes, &new_cld);

    // non repeated
    } else {
      for (auto& cld : record->asObject()) {
        if (cld.id == parent.first) {
          return insertValue(column, parents, indexes, &cld);
        }
      }

      auto new_cld = record->addChild(parent.first);
      return insertValue(column, parents, indexes, &new_cld);
    }
  }

  fnord::iputs("insert into col: $0, idx: $1, parents: $2", column->field_id, indexes, parents);

  switch (column->field_type) {
    case msg::FieldType::OBJECT:
      break;

    case msg::FieldType::STRING:
      record->addChild(
          column->field_id,
          String((char*) column->data, column->size));
      break;


  }
}

void RecordMaterializer::insertNull(
    ColumnState* column,
    const Vector<size_t>& indexes,
    msg::MessageObject* record) {
}


void RecordMaterializer::ColumnState::fetchIfNotPending() {
  if (pending) {
    return;
  }

  defined = reader->next(&r, &d, &data, &size);
  pending = true;
}

void RecordMaterializer::ColumnState::consume() {
  pending = false;
}


} // namespace cstable
} // namespace fnord

