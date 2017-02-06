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
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/util/protobuf/MessageObject.h>
#include <eventql/io/cstable/cstable.h>
#include <eventql/io/cstable/io/PageReader.h>

namespace cstable {
class ColumnWriter;
class ColumnStorage;

class ColumnReader : public RefCounted {
public:

  enum class Visibility { SHARED, PRIVATE };

  virtual bool readBoolean(
      uint64_t* rlvl,
      uint64_t* dlvl,
      bool* value) = 0;

  virtual bool readUnsignedInt(
      uint64_t* rlvl,
      uint64_t* dlvl,
      uint64_t* value) = 0;

  virtual bool readSignedInt(
      uint64_t* rlvl,
      uint64_t* dlvl,
      int64_t* value) = 0;

  virtual bool readFloat(
      uint64_t* rlvl,
      uint64_t* dlvl,
      double* value) = 0;

  virtual bool readString(
      uint64_t* rlvl,
      uint64_t* dlvl,
      String* value) = 0;

  virtual bool readDateTime(
      uint64_t* rlvl,
      uint64_t* dlvl,
      UnixTime* value);

  virtual void readValues(
      size_t n,
      ColumnStorage* dst,
      std::vector<uint32_t>* dlevels = nullptr,
      std::vector<uint32_t>* rlevels = nullptr);

  virtual void readValues(
      size_t n,
      ColumnStorage* dst,
      std::vector<bool>* null_set,
      std::vector<uint32_t>* rlevels = nullptr);

  virtual void skipValue() = 0;
  virtual void copyValue(ColumnWriter* writer) = 0;

  virtual uint64_t nextRepetitionLevel() = 0;

  virtual ColumnType type() const = 0;
  virtual ColumnEncoding encoding() const = 0;

  virtual uint64_t maxRepetitionLevel() const = 0;
  virtual uint64_t maxDefinitionLevel() const = 0;

  virtual void rewind() = 0;

};

class DefaultColumnReader : public ColumnReader {
public:

  DefaultColumnReader(
      ColumnConfig config,
      ScopedPtr<UnsignedIntPageReader> rlevel_reader,
      ScopedPtr<UnsignedIntPageReader> dlevel_reader);

  uint64_t nextRepetitionLevel() override;

  ColumnType type() const override;
  ColumnEncoding encoding() const override;

  uint64_t maxRepetitionLevel() const override;
  uint64_t maxDefinitionLevel() const override;

protected:
  ColumnConfig config_;
  ScopedPtr<UnsignedIntPageReader> rlevel_reader_;
  ScopedPtr<UnsignedIntPageReader> dlevel_reader_;
};

class ColumnStorage {
public:
  virtual ~ColumnStorage() = default;
  virtual void* data() = 0;
  virtual size_t size() = 0;
  virtual void resize(size_t new_size) = 0;
};

class FixedColumnStorage : public ColumnStorage {
public:
  FixedColumnStorage(void* data, size_t* size);
  void* data() override;
  size_t size() override;
  void resize(size_t new_size) override;
protected:
  void* data_;
  size_t* size_;
  size_t size_max_;
};


} // namespace cstable


