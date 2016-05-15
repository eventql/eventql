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
#pragma once
#include "eventql/util/stdtypes.h"
#include "eventql/util/io/inputstream.h"
#include "eventql/util/io/outputstream.h"
#include "eventql/util/UnixTime.h"

#include "eventql/eventql.h"

namespace eventql {

struct LogfileScanLine {
  UnixTime time;
  String raw;
  Vector<String> columns;
};

class LogfileScanResult {
public:

  LogfileScanResult(size_t max_lines = 1000);

  /**
   * Returns a pointer to a mutable (empty) logline if the line was inserted
   * and returns a null pointer if the line was not inserted (e.g. if the
   * result list is already full and this line is too old
   */
  LogfileScanLine* addLine(UnixTime time);

  /**
   * Returns the lines in the result list sorted from newest (first) to oldest
   * (last)
   */
  const List<LogfileScanLine>& lines() const;

  /**
   * Returns true if the result is full, false otherwise
   */
  bool isFull() const;

  /**
   * Returns the max number of rows this result can hold
   */
  size_t capacity() const;

  UnixTime scannedUntil() const;
  void setScannedUntil(UnixTime time);

  size_t rowScanned() const;
  void incrRowsScanned(size_t nrows);

  const Vector<String> columns() const;
  void setColumns(const Vector<String> cols);

  void encode(OutputStream* os) const;
  void decode(InputStream* os);

protected:
  size_t max_lines_;
  List<LogfileScanLine> lines_;
  UnixTime scanned_until_;
  size_t rows_scanned_;
  Vector<String> columns_;
};

} // namespace eventql
