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
#ifndef _FNORD_TSDB_DERIVEDDATASET_H
#define _FNORD_TSDB_DERIVEDDATASET_H
#include <eventql/util/stdtypes.h>
#include <eventql/util/option.h>
#include <eventql/util/autoref.h>
#include <eventql/util/util/binarymessagereader.h>
#include <eventql/util/util/binarymessagewriter.h>

using namespace stx;

namespace eventql {
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
