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
#include <eventql/io/cstable/cstable.h>
#include <eventql/io/cstable/page_manager.h>
#include <eventql/io/cstable/io/PageWriter.h>

namespace cstable {

class LEB128PageWriter : public UnsignedIntPageWriter {
public:
  static const uint64_t kPageSize = 512 * 1024;

  LEB128PageWriter(
      PageIndexKey key,
      PageManager* page_mgr);

  void appendValue(uint64_t value) override;

protected:
  PageIndexKey key_;
  PageManager* page_mgr_;
  bool has_page_;
  size_t page_pos_;
  cstable::PageRef page_;
};

} // namespace cstable


