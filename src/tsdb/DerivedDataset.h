/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_TSDB_DERIVEDDATASET_H
#define _FNORD_TSDB_DERIVEDDATASET_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/option.h>
#include <fnord-base/autoref.h>
#include <fnord-base/util/binarymessagereader.h>
#include <fnord-base/util/binarymessagewriter.h>

using namespace fnord;

namespace tsdb {
class RecordSet;

struct DerivedDatasetState {
  DerivedDatasetState();

  uint64_t last_offset;
  Buffer state;
  void encode(util::BinaryMessageWriter* writer) const;
  void decode(util::BinaryMessageReader* reader);
};

class DerivedDataset : public RefCounted {
public:

  virtual ~DerivedDataset() {}

  virtual String name() = 0;

  virtual void update(
      RecordSet* records,
      uint64_t last_offset,
      uint64_t current_offset,
      const Buffer& last_state,
      Buffer* new_state,
      Set<String>* delete_after_commit) = 0;

};

}
#endif
