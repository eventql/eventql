/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "eventql/TimeseriesResult.h"

using namespace stx;

namespace zbase {

template <typename PointType>
void TimeseriesResult<PointType>::merge(
    const AnalyticsQueryResult::SubQueryResult& o) {
  const auto& other = dynamic_cast<const TimeseriesResult&>(o);

  for (int i = 0; i < other.segment_keys.size(); ++i) {
    if (i >= segment_keys.size()) {
      segment_keys.emplace_back(other.segment_keys[i]);
      series.emplace_back();
    } else if (segment_keys[i] != other.segment_keys[i]) {
      RAISE(kIllegalStateError, "segment keys do not match");
    }

    auto& s_dst = series[i];
    for (const auto& p : other.series[i]) {
      s_dst[p.first].merge(p.second);
    }
  }
}

template <typename PointType>
void TimeseriesResult<PointType>::toJSON(
    json::JSONOutputStream* json) const {
  json->beginArray();

  for (int n = 0; n < segment_keys.size(); ++n) {
    PointType aggr;

    for (const auto& p : series[n]) {
      aggr.merge(p.second);
    }

    if (n > 0) {
      json->addComma();
    }

    json->beginObject();
    json->addObjectEntry("segment");
    json->addString(segment_keys[n]);
    json->addComma();
    json->addObjectEntry("aggregate");
    aggr.toJSON(json);
    json->addComma();
    json->addObjectEntry("timeseries");
    json->beginArray();

    int i = 0;
    for (const auto& p : series[n]) {
      if (++i > 1) {
        json->addComma();
      }

      json->beginObject();
      json->addObjectEntry("time");
      json->addInteger(p.first);
      json->addComma();
      json->addObjectEntry("data");
      p.second.toJSON(json);
      json->endObject();
    }

    json->endArray();
    json->endObject();
  }

  json->endArray();
}

template <typename PointType>
void TimeseriesResult<PointType>::encode(
    util::BinaryMessageWriter* writer) const {
  uint64_t n = 0;
  for (const auto& s : series) {
    n += s.size();
  }

  writer->appendUInt64(n);
  for (int i = 0; i < series.size(); ++i) {
    for (const auto& p : series[i]) {
      writer->appendUInt16(i);
      writer->appendUInt64(p.first);
      p.second.encode(writer);
    }
  }
}

template <typename PointType>
void TimeseriesResult<PointType>::decode(util::BinaryMessageReader* reader) {
  auto n = *reader->readUInt64();
  for (int i = 0; i < n; ++i) {
    auto idx = *reader->readUInt16();
    auto time = *reader->readUInt64();
    PointType p;
    p.decode(reader);
    series[idx][time].merge(p);
  }
}

template <typename PointType>
void TimeseriesResult<PointType>::applyTimeRange(
    UnixTime from,
    UnixTime until) {
  for (auto& s : series) {
    for (auto p = s.begin(); p != s.end(); ) {
      if (p->first * kMicrosPerSecond >= from.unixMicros() &&
          p->first * kMicrosPerSecond < until.unixMicros()) {
        ++p;
      } else {
        p = s.erase(p);
      }
    }
  }
}

template <typename PointType>
void TimeseriesDrilldownResult<PointType>::merge(
    const AnalyticsQueryResult::SubQueryResult& o) {
  const auto& other = dynamic_cast<const TimeseriesDrilldownResult&>(o);
  timeseries.merge(other.timeseries);
  drilldown.merge(other.drilldown);
}

template <typename PointType>
void TimeseriesDrilldownResult<PointType>::encode(
    util::BinaryMessageWriter* writer) const {
  timeseries.encode(writer);
  drilldown.encode(writer);
}

template <typename PointType>
void TimeseriesDrilldownResult<PointType>::decode(
    util::BinaryMessageReader* reader) {
  timeseries.decode(reader);
  drilldown.decode(reader);
}

template <typename PointType>
void TimeseriesDrilldownResult<PointType>::toJSON(
    json::JSONOutputStream* json) const {
  json->beginObject();

  json->addObjectEntry("timeseries");
  timeseries.toJSON(json);
  json->addComma();

  json->addObjectEntry("drilldown");
  drilldown.toJSON(json);

  json->endObject();
}

template <typename PointType>
void TimeseriesDrilldownResult<PointType>::applyTimeRange(
    UnixTime from,
    UnixTime until) {
  timeseries.applyTimeRange(from, until);
}

template <typename PointType>
void TimeseriesBreakdownResult<PointType>::merge(
    const AnalyticsQueryResult::SubQueryResult& o) {
  const auto& other = dynamic_cast<
      const TimeseriesBreakdownResult<PointType>&>(o);

  for (const auto& tp : other.timeseries) {
    auto& ts = timeseries[tp.first];

    for (const auto& p : tp.second) {
      ts[p.first].merge(p.second);
    }
  }
}

template <typename PointType>
void TimeseriesBreakdownResult<PointType>::toJSON(
    json::JSONOutputStream* json) const {
  json->beginObject();
  json->addObjectEntry("timeseries");
  json->beginArray();

  int i = 0;
  for (const auto& tp : timeseries) {
    for (const auto& p : tp.second) {
      if (++i > 1) {
        json->addComma();
      }


      json->beginObject();
      json->addObjectEntry("time");
      json->addInteger(tp.first);
      json->addComma();
      json->addObjectEntry("dimensions");
      json->beginObject();
      auto dims = StringUtil::split(p.first, "~");
      size_t jn = 0;
      for (size_t j = 0; j < dims.size(); ++j) {
        if (dims[j].empty()) {
          continue;
        }

        if (++jn > 1) {
          json->addComma();
        }

        json->addObjectEntry(
            dimensions.size() > j ? dimensions[j] : StringUtil::toString(j));
        json->addString(dims[j]);
      }
      json->endObject();
      json->addComma();
      json->addObjectEntry("data");
      p.second.toJSON(json);
      json->endObject();
    }
  }

  json->endArray();
  json->endObject();
}

template <typename PointType>
void TimeseriesBreakdownResult<PointType>::encode(
    util::BinaryMessageWriter* writer) const {
  size_t n = 0;

  for (const auto& tp : timeseries) {
    for (const auto& p : tp.second) {
      ++n;
    }
  }

  writer->appendUInt64(n);

  for (const auto& tp : timeseries) {
    for (const auto& p : tp.second) {
      writer->appendUInt64(tp.first);
      writer->appendLenencString(p.first);
      p.second.encode(writer);
    }
  }
}

template <typename PointType>
void TimeseriesBreakdownResult<PointType>::decode(
    util::BinaryMessageReader* reader) {
  auto n = *reader->readUInt64();
  for (int i = 0; i < n; ++i) {
    auto time = *reader->readUInt64();
    auto dim = reader->readLenencString();

    PointType p;
    p.decode(reader);

    timeseries[time][dim].merge(p);
  }
}

template <typename PointType>
void TimeseriesBreakdownResult<PointType>::applyTimeRange(
    UnixTime from,
    UnixTime until) {
  for (auto p = timeseries.begin(); p != timeseries.end(); ) {
    if (p->first * kMicrosPerSecond >= from.unixMicros() &&
        p->first * kMicrosPerSecond < until.unixMicros()) {
      ++p;
    } else {
      p = timeseries.erase(p);
    }
  }
}


} // namespace zbase

