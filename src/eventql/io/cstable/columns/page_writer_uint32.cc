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
#include <eventql/io/cstable/columns/page_writer_uint32.h>
#include <eventql/util/inspect.h>

namespace cstable {

UInt32PageWriter::UInt32PageWriter(
    PageIndexKey key,
    PageManager* page_mgr) :
    key_(key),
    page_mgr_(page_mgr),
    has_page_(false),
    page_pos_(0) {}

void UInt32PageWriter::appendValue(uint64_t value) {
  if (!has_page_ || page_pos_ + sizeof(uint32_t) > page_.size) {
    if (has_page_) {
      page_mgr_->flushPage(page_);
    }

    page_ = page_mgr_->allocPage(key_, kPageSize);
    page_pos_ = 0;
    has_page_ = true;
  }

  uint32_t v = value;
  page_mgr_->writeToPage(page_, page_pos_, &v, sizeof(uint32_t));
  page_pos_ += sizeof(uint32_t);
}

} // namespace cstable


