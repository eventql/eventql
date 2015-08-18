/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
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
