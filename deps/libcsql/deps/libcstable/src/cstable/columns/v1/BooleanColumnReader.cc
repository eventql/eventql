/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <cstable/columns/v1/BooleanColumnReader.h>
#include <cstable/ColumnWriter.h>

namespace cstable {
namespace v1 {

BooleanColumnReader::BooleanColumnReader(
    uint64_t r_max,
    uint64_t d_max,
    void* data,
    size_t size) :
    ColumnReader(r_max, d_max, data, size),
    data_reader_(data_, data_size_, 1) {}

bool BooleanColumnReader::readBoolean(
    uint64_t* rlvl,
    uint64_t* dlvl,
    bool* value) {
  uint64_t tmp;
  if (readUnsignedInt(rlvl, dlvl, &tmp)) {
    *value = tmp > 0;
    return true;
  } else {
    *value = false;
    return false;
  }
}

bool BooleanColumnReader::readUnsignedInt(
    uint64_t* rlvl,
    uint64_t* dlvl,
    uint64_t* value) {
  auto r = rlvl_reader_.next();
  auto d = dlvl_reader_.next();

  *rlvl = r;
  *dlvl = d;
  ++vals_read_;

  if (d == d_max_) {
    *value = data_reader_.next();
    return true;
  } else {
    *value = false;
    return false;
  }
}

bool BooleanColumnReader::readSignedInt(
    uint64_t* rlvl,
    uint64_t* dlvl,
    int64_t* value) {
  uint64_t tmp;
  if (readUnsignedInt(rlvl, dlvl, &tmp)) {
    *value = tmp;
    return true;
  } else {
    *value = 0;
    return false;
  }
}

bool BooleanColumnReader::readFloat(
    uint64_t* rlvl,
    uint64_t* dlvl,
    double* value) {
  uint64_t tmp;
  if (readUnsignedInt(rlvl, dlvl, &tmp)) {
    *value = tmp;
    return true;
  } else {
    *value = 0;
    return false;
  }
}

bool BooleanColumnReader::readString(
    uint64_t* rlvl,
    uint64_t* dlvl,
    String* value) {
  uint64_t tmp;
  if (readUnsignedInt(rlvl, dlvl, &tmp)) {
    *value = (value > 0 ? "true" : "false");
    return true;
  } else {
    *value = "";
    return false;
  }
}

void BooleanColumnReader::skipValue() {
  uint64_t rlvl;
  uint64_t dlvl;
  bool val;
  readBoolean(&rlvl, &dlvl, &val);
}

void BooleanColumnReader::copyValue(ColumnWriter* writer) {
  uint64_t rlvl;
  uint64_t dlvl;
  bool val;
  readBoolean(&rlvl, &dlvl, &val);
  if (dlvl == d_max_) {
    writer->writeBoolean(rlvl, dlvl, val);
  } else {
    writer->writeNull(rlvl, dlvl);
  }
}

} // namespace v1
} // namespace cstable

