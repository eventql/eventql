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
#include <eventql/io/cstable/columns/v1/StringColumnReader.h>
#include <eventql/io/cstable/ColumnWriter.h>

namespace cstable {
namespace v1 {

StringColumnReader::StringColumnReader(
    uint64_t r_max,
    uint64_t d_max,
    void* data,
    size_t size) :
    ColumnReader(r_max, d_max, data, size),
    data_reader_(data_, data_size_) {}

bool StringColumnReader::readBoolean(
    uint64_t* rlvl,
    uint64_t* dlvl,
    bool* value) {
  String tmp;
  if (readString(rlvl, dlvl, &tmp)) {
    *value = (tmp == "true");
    return true;
  } else {
    *value = 0;
    return false;
  }
}

bool StringColumnReader::readUnsignedInt(
    uint64_t* rlvl,
    uint64_t* dlvl,
    uint64_t* value) {
  String tmp;
  if (readString(rlvl, dlvl, &tmp)) {
    *value = std::stoull(tmp);
    return true;
  } else {
    *value = 0;
    return false;
  }
}

bool StringColumnReader::readSignedInt(
    uint64_t* rlvl,
    uint64_t* dlvl,
    int64_t* value) {
  String tmp;
  if (readString(rlvl, dlvl, &tmp)) {
    *value = std::stoll(tmp);
    return true;
  } else {
    *value = 0;
    return false;
  }
}

bool StringColumnReader::readFloat(
    uint64_t* rlvl,
    uint64_t* dlvl,
    double* value) {
  String tmp;
  if (readString(rlvl, dlvl, &tmp)) {
    *value = std::stod(tmp);
    return true;
  } else {
    *value = 0;
    return false;
  }
}

bool StringColumnReader::readString(
    uint64_t* rlvl,
    uint64_t* dlvl,
    String* value) {
  auto r = rlvl_reader_.next();
  auto d = dlvl_reader_.next();

  *rlvl = r;
  *dlvl = d;
  ++vals_read_;

  if (d == d_max_) {
    auto data_len = *data_reader_.readUInt32();
    auto data = data_reader_.readString(data_len);
    *value = String(data, data_len);
    return true;
  } else {
    *value = "";
    return false;
  }
}

void StringColumnReader::skipValue() {
  uint64_t rlvl;
  uint64_t dlvl;
  String val;
  readString(&rlvl, &dlvl, &val);
}

void StringColumnReader::copyValue(ColumnWriter* writer) {
  uint64_t rlvl;
  uint64_t dlvl;
  String val;
  readString(&rlvl, &dlvl, &val);
  if (dlvl == d_max_) {
    writer->writeString(rlvl, dlvl, val);
  } else {
    writer->writeNull(rlvl, dlvl);
  }
}

} // namespace v1
} // namespace cstable

