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
#ifndef _FNORD_SSTABLE_INDEX_H
#define _FNORD_SSTABLE_INDEX_H
#include <stdlib.h>
#include <string>
#include <vector>
#include <memory>

namespace stx {
namespace sstable {

class Index {
public:
  typedef std::unique_ptr<Index> IndexRef;

  Index(uint32_t type);
  Index(const Index& copy) = delete;
  Index& operator=(const Index& copy) = delete;
  virtual ~Index() {}

  uint32_t type() const;

  virtual void addRow(
      size_t body_offset,
      void const* key,
      size_t key_size,
      void const* data,
      size_t data_size) const = 0;

protected:
  uint32_t type_;
};


}
}

#endif
