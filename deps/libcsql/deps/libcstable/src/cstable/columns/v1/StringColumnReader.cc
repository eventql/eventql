/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <cstable/columns/v1/StringColumnReader.h>
#include <cstable/ColumnWriter.h>

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

