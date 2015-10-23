/**
 * This file is part of the "tsdb" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/autoref.h>
#include <stx/option.h>
#include <stx/protobuf/MessageObject.h>
#include <zbase/core/PartitionSnapshot.h>

using namespace stx;

namespace zbase {
class Partition;

class PartitionReader : public RefCounted {
public:

  PartitionReader(RefPtr<PartitionSnapshot> head);

  Option<RefPtr<VFSFile>> fetchCSTable() const;
  Option<String> fetchCSTableFilename() const;
  Option<SHA1Hash> cstableVersion() const;
  Option<String> cstableFilename() const;

  virtual void fetchRecords(
      Function<void (const msg::MessageObject& record)> fn) = 0;

protected:
  RefPtr<PartitionSnapshot> snap_;
};

} // namespace tdsb

