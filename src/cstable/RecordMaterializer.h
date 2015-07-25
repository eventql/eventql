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
#include <stx/stdtypes.h>
#include <stx/io/file.h>
#include <stx/io/mmappedfile.h>
#include <cstable/BinaryFormat.h>
#include <cstable/CSTableReader.h>
#include <cstable/ColumnReader.h>
#include <stx/protobuf/MessageSchema.h>

namespace stx {
namespace cstable {

class RecordMaterializer {
public:

  RecordMaterializer(
      msg::MessageSchema* schema,
      CSTableReader* reader,
      Set<String> columns = Set<String> {});

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
    Vector<Tuple<uint64_t, bool, uint32_t>> parents;
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
      Vector<Tuple<uint64_t, bool, uint32_t>> parents,
      Vector<size_t> indexes,
      msg::MessageObject* record);

  void insertNull(
      ColumnState* column,
      Vector<Tuple<uint64_t, bool, uint32_t>> parents,
      const Vector<size_t> indexes,
      msg::MessageObject* record);

  void createColumns(
      const String& prefix,
      uint32_t dmax,
      Vector<Tuple<uint64_t, bool, uint32_t>> parents,
      const msg::MessageSchemaField& field,
      CSTableReader* reader,
      const Set<String>& columns);

  HashMap<String, ColumnState> columns_;
};

} // namespace cstable
} // namespace stx

#endif
