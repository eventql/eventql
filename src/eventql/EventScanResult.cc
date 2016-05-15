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
#include "eventql/util/protobuf/MessageEncoder.h"
#include "eventql/util/protobuf/MessageDecoder.h"
#include "eventql/EventScanResult.h"

using namespace stx;

namespace eventql {

EventScanRow::EventScanRow(RefPtr<msg::MessageSchema> schema) : obj(schema) {}

EventScanResult::EventScanResult(
    RefPtr<msg::MessageSchema> schema,
    size_t max_rows /* = 1000 */) :
    schema_(schema),
    max_rows_(max_rows),
    scanned_until_(WallClock::unixMicros()),
    rows_scanned_(0) {}

EventScanRow* EventScanResult::addRow(UnixTime time) {
  auto nrows = rows_.size();

  // fast path if time is older than any stored time
  if (nrows > 0 &&
      time.unixMicros() <= rows_.back().time.unixMicros()) {
    if (nrows < max_rows_) {
      rows_.emplace_back(schema_);
      auto line = &rows_.back();
      line->time = time;
      return line;
    } else {
      return nullptr;
    }
  }

  auto cur = rows_.begin();
  auto end = rows_.end();
  while (cur != end && time.unixMicros() < cur->time.unixMicros()) {
    ++cur;
  }

  auto line = rows_.emplace(cur, schema_);
  line->time = time;

  while (rows_.size() > max_rows_) {
    rows_.pop_back();
  }

  return &*line;
}

RefPtr<msg::MessageSchema> EventScanResult::schema() const {
  return schema_;
}

const List<EventScanRow>& EventScanResult::rows() const {
  return rows_;
}

bool EventScanResult::isFull() const {
  return rows_.size() >= max_rows_;
}

size_t EventScanResult::capacity() const {
  return max_rows_;
}

UnixTime EventScanResult::scannedUntil() const {
  return scanned_until_;
}

void EventScanResult::setScannedUntil(UnixTime time) {
  scanned_until_ = time;
}

size_t EventScanResult::rowScanned() const {
  return rows_scanned_;
}

void EventScanResult::incrRowsScanned(size_t nrows) {
  rows_scanned_ += nrows;
}

void EventScanResult::encode(OutputStream* os) const {
  os->appendVarUInt(rows_scanned_);
  os->appendVarUInt(rows_.size());

  for (const auto& r : rows_) {
    Buffer buf;
    msg::MessageEncoder::encode(r.obj.data(), *schema_, &buf);
    os->appendVarUInt(r.time.unixMicros());
    os->appendVarUInt(buf.size());
    os->write(buf);
  }
}

void EventScanResult::decode(InputStream* is) {
  rows_scanned_ += is->readVarUInt();

  auto nrows = is->readVarUInt();
  for (size_t j = 0; j < nrows; ++j) {
    auto row = addRow(is->readVarUInt());
    auto record_size = is->readVarUInt();

    if (row) {
      Buffer buf(record_size);
      is->readNextBytes(buf.data(), buf.size());

      msg::MessageObject msg;
      msg::MessageDecoder::decode(buf, *schema_, &msg);

      row->obj.setData(msg);
    } else {
      is->skipNextBytes(record_size);
    }
  }
}

} // namespace eventql
