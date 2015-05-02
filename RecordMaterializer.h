/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_CSTABLE_RECORDMATERIALIZER_H
#define _FNORD_CSTABLE_RECORDMATERIALIZER_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/io/file.h>
#include <fnord-base/io/mmappedfile.h>
#include <fnord-cstable/BinaryFormat.h>
#include <fnord-cstable/CSTableReader.h>
#include <fnord-cstable/ColumnReader.h>
#include <fnord-msg/MessageSchema.h>

namespace fnord {
namespace cstable {

class RecordMaterializer {
public:

  RecordMaterializer(
      msg::MessageSchema* schema,
      CSTableReader* reader);

  void nextRecord(msg::MessageObject* record);
  void skipRecord();

protected:

  struct ColumnState {
    ColumnState(RefPtr<cstable::ColumnReader> _reader) :
        reader(_reader),
        data(nullptr),
        size(0),
        r(0),
        d(0),
        pending(false),
        defined(false) {}

    RefPtr<cstable::ColumnReader> reader;
    void* data;
    size_t size;
    uint64_t r;
    uint64_t d;
    bool pending;
    bool defined;
    Vector<Pair<uint64_t, bool>> parents;
    uint32_t field_id;
    msg::FieldType field_type;

    void fetchIfNotPending();
    void consume();
  };

  void loadColumn(
      const String& column_name,
      ColumnState* column,
      msg::MessageObject* record);

  void insertValue(
      ColumnState* column,
      Vector<Pair<uint64_t, bool>> parents,
      Vector<size_t> indexes,
      msg::MessageObject* record);

  void insertNull(
      ColumnState* column,
      const Vector<size_t> indexes,
      msg::MessageObject* record);

  void createColumns(
      const String& prefix,
      Vector<Pair<uint64_t, bool>> parents,
      const msg::MessageSchemaField& field,
      CSTableReader* reader);

  HashMap<String, ColumnState> columns_;
};

} // namespace cstable
} // namespace fnord

#endif
