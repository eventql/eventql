/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/wallclock.h"
#include "stx/protobuf/MessageEncoder.h"
#include "stx/protobuf/MessageDecoder.h"
#include "eventql/EventScanResult.h"

using namespace stx;

namespace zbase {

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

} // namespace zbase
