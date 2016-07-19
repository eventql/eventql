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
#ifndef _FNORD_CSTABLE_STRINGCOLUMNREADER_H
#define _FNORD_CSTABLE_STRINGCOLUMNREADER_H
#include <eventql/util/stdtypes.h>
#include <eventql/util/util/binarymessagereader.h>
#include <eventql/util/util/BitPackDecoder.h>
#include <eventql/io/cstable/columns/v1/ColumnReader.h>


namespace cstable {
namespace v1 {

class StringColumnReader : public ColumnReader {
public:

  StringColumnReader(
      uint64_t r_max,
      uint64_t d_max,
      void* data,
      size_t size);

  bool readBoolean(
      uint64_t* rlvl,
      uint64_t* dlvl,
      bool* value) override;

  bool readUnsignedInt(
      uint64_t* rlvl,
      uint64_t* dlvl,
      uint64_t* value) override;

  bool readSignedInt(
      uint64_t* rlvl,
      uint64_t* dlvl,
      int64_t* value) override;

  bool readFloat(
      uint64_t* rlvl,
      uint64_t* dlvl,
      double* value) override;

  bool readString(
      uint64_t* rlvl,
      uint64_t* dlvl,
      String* value) override;

  void skipValue() override;
  void copyValue(ColumnWriter* writer) override;

  ColumnType type() const override {
    return ColumnType::STRING;
  }

  ColumnEncoding encoding() const override {
    return ColumnEncoding::STRING_PLAIN;
  }

  void rewind() override {
    rlvl_reader_.rewind();
    dlvl_reader_.rewind();
    data_reader_.rewind();
  }

protected:
  util::BinaryMessageReader data_reader_;
};

} // namespace v1
} // namespace cstable


#endif
