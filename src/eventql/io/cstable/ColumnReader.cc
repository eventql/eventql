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
#include <assert.h>
#include <eventql/io/cstable/ColumnReader.h>

namespace cstable {

bool ColumnReader::readDateTime(
    uint64_t* rlvl,
    uint64_t* dlvl,
    UnixTime* value) {
  uint64_t tmp;
  if (readUnsignedInt(rlvl, dlvl, &tmp)) {
    *value = UnixTime(tmp);
    return true;
  } else {
    *value = UnixTime(0);
    return false;
  }
}

DefaultColumnReader::DefaultColumnReader(
    ColumnConfig config,
    ScopedPtr<UnsignedIntPageReader> rlevel_reader,
    ScopedPtr<UnsignedIntPageReader> dlevel_reader) :
    config_(config),
    rlevel_reader_(std::move(rlevel_reader)),
    dlevel_reader_(std::move(dlevel_reader)) {}

uint64_t DefaultColumnReader::maxRepetitionLevel() const {
  return config_.rlevel_max;
}

uint64_t DefaultColumnReader::maxDefinitionLevel() const {
  return config_.dlevel_max;
}

ColumnType DefaultColumnReader::type() const {
  return config_.logical_type;
}

ColumnEncoding DefaultColumnReader::encoding() const {
  return config_.storage_type;
}

uint64_t DefaultColumnReader::nextRepetitionLevel() {
  if (config_.rlevel_max > 0) {
    return rlevel_reader_->peek();
  } else {
    return 0;
  }
}

FixedColumnStorage::FixedColumnStorage(
    void* data,
    size_t* size) :
    data_(data),
    size_(size),
    size_max_(*size) {}

void* FixedColumnStorage::data() {
  return data_;
}

size_t FixedColumnStorage::size() {
  return *size_;
}

void FixedColumnStorage::resize(size_t new_size) {
  assert(new_size <= size_max_);
  *size_ = new_size;
}

} // namespace cstable

