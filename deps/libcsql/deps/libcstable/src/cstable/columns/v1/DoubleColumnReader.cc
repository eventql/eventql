/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <cstable/columns/v1/DoubleColumnReader.h>
#include <cstable/ColumnWriter.h>

using namespace stx;

namespace cstable {
namespace v1 {

DoubleColumnReader::DoubleColumnReader(
    uint64_t r_max,
    uint64_t d_max,
    void* data,
    size_t size) :
    ColumnReader(r_max, d_max, data, size),
    data_reader_(data_, data_size_) {}

bool DoubleColumnReader::readBoolean(
    uint64_t* rlvl,
    uint64_t* dlvl,
    bool* value) {
  double tmp;
  if (readFloat(rlvl, dlvl, &tmp)) {
    *value = tmp > 1;
    return true;
  } else {
    *value = false;
    return false;
  }
}

bool DoubleColumnReader::readUnsignedInt(
    uint64_t* rlvl,
    uint64_t* dlvl,
    uint64_t* value) {
  double tmp;
  if (readFloat(rlvl, dlvl, &tmp)) {
    *value = tmp;
    return true;
  } else {
    *value = 0;
    return false;
  }
}

bool DoubleColumnReader::readSignedInt(
    uint64_t* rlvl,
    uint64_t* dlvl,
    int64_t* value) {
  double tmp;
  if (readFloat(rlvl, dlvl, &tmp)) {
    *value = tmp;
    return true;
  } else {
    *value = 0;
    return false;
  }
}

bool DoubleColumnReader::readFloat(
    uint64_t* rlvl,
    uint64_t* dlvl,
    double* value) {
  auto r = rlvl_reader_.next();
  auto d = dlvl_reader_.next();

  *rlvl = r;
  *dlvl = d;
  ++vals_read_;

  if (d == d_max_) {
    *value = data_reader_.readDouble();
    return true;
  } else {
    *value = 0;
    return false;
  }
}

bool DoubleColumnReader::readString(
    uint64_t* rlvl,
    uint64_t* dlvl,
    String* value) {
  double tmp;
  if (readFloat(rlvl, dlvl, &tmp)) {
    *value = StringUtil::toString(tmp);
    return true;
  } else {
    *value = "";
    return false;
  }
}

void DoubleColumnReader::skipValue() {
  uint64_t rlvl;
  uint64_t dlvl;
  double val;
  readFloat(&rlvl, &dlvl, &val);
}

#include <stx/inspect.h>
void DoubleColumnReader::copyValue(ColumnWriter* writer) {
  uint64_t rlvl;
  uint64_t dlvl;
  double val;
  readFloat(&rlvl, &dlvl, &val);
  if (dlvl == d_max_) {
    writer->writeFloat(rlvl, dlvl, val);
  } else {
    writer->writeNull(rlvl, dlvl);
  }
}

} // namespace v1
} // namespace cstable

