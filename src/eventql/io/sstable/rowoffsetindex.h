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
#ifndef _FNORD_SSTABLE_ROWOFFSETINDEX_H
#define _FNORD_SSTABLE_ROWOFFSETINDEX_H
#include <eventql/io/sstable/index.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <memory>


namespace sstable {

class RowOffsetIndex : public Index {
public:
  static const uint32_t kIndexType = 0xa0a0;

  static RowOffsetIndex* makeIndex();

  RowOffsetIndex();

  void addRow(
      size_t body_offset,
      void const* key,
      size_t key_size,
      void const* data,
      size_t data_size) const override;

};


}

#endif
