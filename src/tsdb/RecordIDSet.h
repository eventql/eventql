/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/io/file.h>
#include <stx/option.h>
#include <stx/SHA1.h>
#include <stx/util/binarymessagereader.h>
#include <stx/util/binarymessagewriter.h>
#include <stx/random.h>

using namespace stx;

namespace tsdb {

class RecordIDSet {
public:

  RecordIDSet(
      const String& datadir,
      const String& filename);

  void addRecordID(const SHA1Hash& record_id);
  bool hasRecordID(const SHA1Hash& record_id);

  Set<SHA1Hash> fetchRecordIDs();

protected:
  String datadir_;
  String filename_prefix_;
  mutable std::mutex mutex_;
};

} // namespace tdsb

