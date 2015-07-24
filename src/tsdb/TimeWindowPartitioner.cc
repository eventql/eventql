/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord/util/binarymessagereader.h>
#include <fnord/util/binarymessagewriter.h>
#include <tsdb/TimeWindowPartitioner.h>

using namespace fnord;

namespace tsdb {

SHA1Hash TimeWindowPartitioner::partitionKeyFor(
    const String& stream_key,
    UnixTime time,
    Duration window_size) {
  util::BinaryMessageWriter buf(stream_key.size() + 32);

  auto cs = window_size.microseconds();
  auto ts = (time.unixMicros() / cs) * cs / kMicrosPerSecond;

  buf.append(stream_key.data(), stream_key.size());
  buf.appendUInt8(27);
  buf.appendVarUInt(ts);

  return SHA1::compute(buf.data(), buf.size());
}

Vector<SHA1Hash> TimeWindowPartitioner::partitionKeysFor(
    const String& stream_key,
    UnixTime from,
    UnixTime until,
    Duration window_size) {
  auto cs = window_size.microseconds();
  auto first_chunk = (from.unixMicros() / cs) * cs;
  auto last_chunk = (until.unixMicros() / cs) * cs;

  Vector<SHA1Hash> res;
  for (auto t = first_chunk; t <= last_chunk; t += cs) {
    res.emplace_back(partitionKeyFor(stream_key, t, window_size));
  }

  return res;
}


}
