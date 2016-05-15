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
#ifndef _FNORD_CSTABLE_RECORDMATERIALIZER_H
#define _FNORD_CSTABLE_RECORDMATERIALIZER_H
#include <eventql/util/stdtypes.h>
#include <eventql/util/io/file.h>
#include <eventql/util/io/mmappedfile.h>
#include <eventql/infra/cstable/cstable.h>
#include <eventql/infra/cstable/CSTableReader.h>
#include <eventql/infra/cstable/ColumnReader.h>
#include <eventql/util/protobuf/MessageSchema.h>

namespace cstable {

class RecordMaterializer {
public:

  RecordMaterializer(
      stx::msg::MessageSchema* schema,
      CSTableReader* reader,
      Set<String> columns = Set<String> {});

  void nextRecord(stx::msg::MessageObject* record);
  void skipRecord();

protected:

  struct ColumnState {
    ColumnState(RefPtr<cstable::ColumnReader> _reader) :
        reader(_reader),
        r(0),
        d(0),
        pending(false),
        defined(false) {}

    RefPtr<cstable::ColumnReader> reader;
    uint64_t r;
    uint64_t d;
    bool pending;
    bool defined;
    Vector<stx::Tuple<uint64_t, bool, uint32_t>> parents;
    uint32_t field_id;
    stx::msg::FieldType field_type;
    ColumnType col_type;

    String val_str;
    uint64_t val_uint;
    int64_t val_sint;
    double val_float;

    uint64_t getUnsignedInteger() const;
    int64_t getSignedInteger() const;
    String getString() const;
    double getFloat() const;

    void fetchIfNotPending();
    void consume();
  };

  void loadColumn(
      const String& column_name,
      ColumnState* column,
      stx::msg::MessageObject* record);

  void insertValue(
      ColumnState* column,
      Vector<stx::Tuple<uint64_t, bool, uint32_t>> parents,
      Vector<size_t> indexes,
      stx::msg::MessageObject* record);

  void insertNull(
      ColumnState* column,
      Vector<stx::Tuple<uint64_t, bool, uint32_t>> parents,
      const Vector<size_t> indexes,
      stx::msg::MessageObject* record);

  void createColumns(
      const String& prefix,
      uint32_t dmax,
      Vector<stx::Tuple<uint64_t, bool, uint32_t>> parents,
      const stx::msg::MessageSchemaField& field,
      CSTableReader* reader,
      const Set<String>& columns);

  HashMap<String, ColumnState> columns_;
};

} // namespace cstable


#endif
