/**
 * This file is part of the "libcstable" project
 *   Copyright (c) 2014 Paul Asmuth, FnordCorp B.V.
 *
 * libcstable is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/autoref.h>
#include <stx/protobuf/MessageObject.h>
#include <cstable/cstable.h>


namespace cstable {
class ColumnWriter;

class ColumnReader : public stx::RefCounted {
public:

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
  virtual bool eofReached() const = 0;

  virtual ColumnType type() const = 0;
  virtual ColumnEncoding encoding() const = 0;

  virtual uint64_t maxRepetitionLevel() const = 0;
  virtual uint64_t maxDefinitionLevel() const = 0;

};

} // namespace cstable


