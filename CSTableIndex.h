/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_TSDB_CSTABLEINDEX_H
#define _FNORD_TSDB_CSTABLEINDEX_H
#include <fnord-base/stdtypes.h>
#include <fnord-tsdb/DerivedDataset.h>

namespace fnord {
namespace tsdb {

class CSTableIndex : public DerivedDataset {
public:

  String name() override;

  void update(
      RecordSet* records,
      uint64_t last_offset,
      uint64_t current_offset,
      const Buffer& last_state,
      Buffer* new_state,
      Set<String>* delete_after_commit) override;

};

}
}
#endif
