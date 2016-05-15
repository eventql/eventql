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
#include <fnord-logtable/TableChunkSummaryWriter.h>

namespace stx {
namespace logtable {

TableChunkSummaryWriter::TableChunkSummaryWriter(
    const String& output_filename) :
    output_filename_(output_filename),
    n_(0) {}

void TableChunkSummaryWriter::addSummary(
    const String& summary_name,
    void* data,
    size_t size) {
  head_.appendVarUInt(summary_name.size());
  head_.append(summary_name.data(), summary_name.size());
  head_.appendVarUInt(body_.size());
  head_.appendVarUInt(size);
  body_.append(data, size);
  ++n_;
}

void TableChunkSummaryWriter::commit() {
  auto file = File::openFile(output_filename_ ,File::O_WRITE | File::O_CREATE);
  file.write(&n_, sizeof(n_));
  file.write(head_.data(), head_.size());
  file.write(body_.data(), body_.size());
}

}
}
