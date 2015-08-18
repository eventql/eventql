/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_CTRBYGROUPRESULT_IMPL_H
#define _CM_CTRBYGROUPRESULT_IMPL_H

using namespace stx;

namespace cm {

template <typename GroupKeyType>
void CTRByGroupResult<GroupKeyType>::toJSON(
    json::JSONOutputStream* json) const {
  json->beginArray();

  for (int n = 0; n <
      GroupResult<GroupKeyType, CTRCounterData>::segment_keys.size(); ++n) {
    uint64_t total_views = 0;
    uint64_t total_clicks = 0;

    for (const auto& c :
        GroupResult<GroupKeyType, CTRCounterData>::counters[n]) {
      total_views += c.second.num_views;
      total_clicks += c.second.num_clicks;
    }

    if (n > 0) {
      json->addComma();
    }

    json->beginObject();
    json->addObjectEntry("segment");
    json->addString(GroupResult<GroupKeyType, CTRCounterData>::segment_keys[n]);
    json->addComma();
    json->addObjectEntry("data");
    json->beginArray();

    int i = 0;
    for (const auto& c :
        GroupResult<GroupKeyType, CTRCounterData>::counters[n]) {
      if (++i > 1) {
        json->addComma();
      }

      json->beginObject();
      json->addObjectEntry("group");
      json->addString(StringUtil::toString(c.first));
      json->addComma();
      json->addObjectEntry("views");
      json->addInteger(c.second.num_views);
      json->addComma();
      json->addObjectEntry("clicks");
      json->addInteger(c.second.num_clicks);
      json->addComma();
      json->addObjectEntry("clicked");
      json->addInteger(c.second.num_clicked);
      json->addComma();
      json->addObjectEntry("ctr");
      json->addFloat(c.second.num_clicks / (double) c.second.num_views);
      json->addComma();
      json->addObjectEntry("cpq");
      json->addFloat(c.second.num_clicked / (double) c.second.num_views);
      json->addComma();
      json->addObjectEntry("clickshare");
      json->addFloat(c.second.num_clicks / (double) total_clicks);
      json->addComma();
      json->addObjectEntry("viewshare");
      json->addFloat(c.second.num_views / (double) total_views);
      json->endObject();
    }

    json->endArray();
    json->endObject();
  }

  json->endArray();
}

} // namespace cm

#endif
