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
#include <eventql/io/cstable/ColumnReader.h>
#include <eventql/io/cstable/io/PageReader.h>
#include <eventql/io/cstable/page_manager.h>

namespace cstable {

class UnsignedIntColumnReader : public DefaultColumnReader {
public:

  UnsignedIntColumnReader(
      ColumnConfig config,
      ScopedPtr<UnsignedIntPageReader> rlevel_reader,
      ScopedPtr<UnsignedIntPageReader> dlevel_reader,
      const PageManager* page_mgr);

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

  void rewind() override;

protected:
  ScopedPtr<UnsignedIntPageReader> data_reader_;
};

} // namespace cstable


