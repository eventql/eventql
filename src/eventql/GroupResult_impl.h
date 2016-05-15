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
#ifndef _CM_GROUPRESULT_IMPL_H
#define _CM_GROUPRESULT_IMPL_H

using namespace util;

namespace eventql {

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


} // namespace eventql

#endif
