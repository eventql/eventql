/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_GROUPRESULT_IMPL_H
#define _CM_GROUPRESULT_IMPL_H

using namespace stx;

namespace cm {

template <typename GroupKeyType, typename GroupValueType>
void GroupResult<GroupKeyType, GroupValueType>::merge(
    const AnalyticsQueryResult::SubQueryResult& o) {
  const auto& other = dynamic_cast<const GroupResult<GroupKeyType, GroupValueType>&>(o);

  for (int i = 0; i < other.segment_keys.size(); ++i) {
    if (i >= segment_keys.size()) {
      segment_keys.emplace_back(other.segment_keys[i]);
      counters.emplace_back();
    } else if (segment_keys[i] != other.segment_keys[i]) {
      RAISE(kIllegalStateError, "segment keys do not match");
    }

    auto& c_dst = counters[i];
    for (const auto& c : other.counters[i]) {
      c_dst[c.first].merge(c.second);
    }
  }
}

template <typename GroupKeyType, typename GroupValueType>
void GroupResult<GroupKeyType, GroupValueType>::toJSON(
    json::JSONOutputStream* json) const {
  json->beginArray();

  for (int n = 0; n < segment_keys.size(); ++n) {
    if (n > 0) {
      json->addComma();
    }

    json->beginObject();
    json->addObjectEntry("segment");
    json->addString(segment_keys[n]);
    json->addComma();
    json->addObjectEntry("data");
    json->beginArray();

    int i = 0;
    for (const auto& c : counters[n]) {
      if (++i > 1) {
        json->addComma();
      }

      json->beginObject();
      json->addObjectEntry("group");
      json->addString(StringUtil::toString(c.first));
      json->addComma();
      json->addObjectEntry("data");
      c.second.toJSON(json);
      json->endObject();
    }

    json->endArray();
    json->endObject();
  }

  json->endArray();
}

template <typename GroupKeyType, typename GroupValueType>
void GroupResult<GroupKeyType, GroupValueType>::encode(
    util::BinaryMessageWriter* writer) const {
  uint64_t n = 0;
  for (const auto& c : counters) {
    n += c.size();
  }

  writer->appendUInt64(n);
  for (int i = 0; i < counters.size(); ++i) {
    for (const auto& c : counters[i]) {
      writer->appendUInt16(i);
      writer->appendValue<GroupKeyType>(c.first);
      c.second.encode(writer);
    }
  }
}

template <typename GroupKeyType, typename GroupValueType>
void GroupResult<GroupKeyType, GroupValueType>::decode(util::BinaryMessageReader* reader) {
  auto n = *reader->readUInt64();
  for (int i = 0; i < n; ++i) {
    auto idx = *reader->readUInt16();
    auto pos = *reader->readValue<GroupKeyType>();
    GroupValueType c;
    c.decode(reader);
    counters[idx][pos].merge(c);
  }
}


} // namespace cm

#endif
