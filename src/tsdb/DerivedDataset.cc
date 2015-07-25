/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <tsdb/DerivedDataset.h>

using namespace stx;

namespace tsdb {

DerivedDatasetState::DerivedDatasetState() : last_offset(0) {}

void DerivedDatasetState::encode(util::BinaryMessageWriter* writer) const {
  writer->appendVarUInt(last_offset);
  writer->appendVarUInt(state.size());
  if (state.size() > 0) {
    writer->append(state.data(), state.size());
  }
}

void DerivedDatasetState::decode(util::BinaryMessageReader* reader) {
  last_offset = reader->readVarUInt();
  auto buf_len = reader->readVarUInt();
  if (buf_len > 0) {
    state = Buffer(reader->read(buf_len), buf_len);
  }
}

}
