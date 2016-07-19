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
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/util/protobuf/MessageObject.h>
#include <eventql/io/cstable/cstable.h>
#include <eventql/io/cstable/io/PageReader.h>

namespace cstable {
class ColumnWriter;

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

} // namespace cstable


