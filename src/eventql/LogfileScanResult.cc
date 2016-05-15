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
#include "eventql/util/wallclock.h"
#include "eventql/LogfileScanResult.h"

using namespace stx;

namespace eventql {

LogfileScanResult::LogfileScanResult(
    size_t max_lines /* = 1000 */) :
    max_lines_(max_lines),
    scanned_until_(WallClock::unixMicros()),
    rows_scanned_(0) {}

LogfileScanLine* LogfileScanResult::addLine(UnixTime time) {
  auto nlines = lines_.size();

  // fast path if time is older than any stored time
  if (nlines > 0 &&
      time.unixMicros() <= lines_.back().time.unixMicros()) {
    if (nlines < max_lines_) {
      lines_.emplace_back();
      auto line = &lines_.back();
      line->time = time;
      return line;
    } else {
      return nullptr;
    }
  }

  auto cur = lines_.begin();
  auto end = lines_.end();
  while (cur != end && time.unixMicros() < cur->time.unixMicros()) {
    ++cur;
  }

  auto line = lines_.emplace(cur);
  line->time = time;

  while (lines_.size() > max_lines_) {
    lines_.pop_back();
  }

  return &*line;
}

const List<LogfileScanLine>& LogfileScanResult::lines() const {
  return lines_;
}

bool LogfileScanResult::isFull() const {
  return lines_.size() >= max_lines_;
}

size_t LogfileScanResult::capacity() const {
  return max_lines_;
}

UnixTime LogfileScanResult::scannedUntil() const {
  return scanned_until_;
}

void LogfileScanResult::setScannedUntil(UnixTime time) {
  scanned_until_ = time;
}

size_t LogfileScanResult::rowScanned() const {
  return rows_scanned_;
}

void LogfileScanResult::incrRowsScanned(size_t nrows) {
  rows_scanned_ += nrows;
}

const Vector<String> LogfileScanResult::columns() const {
  return columns_;
}

void LogfileScanResult::setColumns(const Vector<String> cols) {
  columns_ = cols;
}

void LogfileScanResult::encode(OutputStream* os) const {
  os->appendVarUInt(rows_scanned_);
  os->appendVarUInt(lines_.size());

  for (const auto& l : lines_) {
    os->appendVarUInt(l.time.unixMicros());
    os->appendLenencString(l.raw);
    os->appendVarUInt(l.columns.size());

    for (const auto& c : l.columns) {
      os->appendLenencString(c);
    }
  }
}

void LogfileScanResult::decode(InputStream* is) {
  rows_scanned_ += is->readVarUInt();

  auto nlines = is->readVarUInt();
  for (size_t j = 0; j < nlines; ++j) {
    auto line = addLine(is->readVarUInt());

    if (line) {
      line->raw = is->readLenencString();
      auto ncols = is->readVarUInt();
      for (size_t i = 0; i < ncols; ++i) {
        line->columns.emplace_back(is->readLenencString());
      }
    } else {
      is->readLenencString();
      auto ncols = is->readVarUInt();
      for (size_t i = 0; i < ncols; ++i) {
        is->readLenencString();
      }
    }
  }
}

} // namespace eventql
