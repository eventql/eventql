/*
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <fnord/base/stringutil.h>
#include <fnord/base/uri.h>
#include <fnord/base/wallclock.h>
#include "logjoinservice.h"

namespace cm {

LogJoinService::LogJoinService() {
}

void LogJoinService::insertLogline(
    CustomerNamespace* customer,
    const std::string& log_line) {
  fnord::URI::ParamList params;
  fnord::URI::parseQueryString(log_line, &params);

  std::string c;
  if (!fnord::URI::getParam(params, "c", &c)) {
    return;
  }

  auto c_s = c.find("~");
  if (c_s == std::string::npos) {
    return;
  }

  std::string uid = c.substr(0, c_s);
  std::string cid = c.substr(c_s + 1, c.length() - c_s - 1);

  if (uid.length() == 0 || cid.length() == 0) {
    return;
  }

  std::vector<std::string> attrs;
  for (const auto& p : params) {
    if (p.first != "c" && p.first != "e" && p.first != "i" && p.first != "is") {
      attrs.emplace_back(fnord::StringUtil::format("$0:$1", p.first, p.second));
    }
  }


  std::string evtype;
  if (!fnord::URI::getParam(params, "e", &evtype) || evtype.length() != 1) {
    return;
  }

  switch (evtype[0]) {
    case 'q': {
      std::vector<ItemRef> items;

      insertQuery(
          customer,
          fnord::DateTime(),
          uid,
          cid,
          items,
          attrs);

      break;
    }

    case 'v': {
      cm::ItemRef itemid;

      insertItemVisit(
          customer,
          fnord::DateTime(),
          uid,
          cid,
          itemid,
          attrs);

      break;
    }

  }
}
void LogJoinService::insertQuery(
    CustomerNamespace* customer,
    const fnord::DateTime& time,
    const std::string& uid,
    const std::string& eid,
    const std::vector<ItemRef>& items,
    std::vector<std::string> attrs) {
  auto session = findOrCreateSession(customer, uid);
  std::lock_guard<std::mutex> lock_holder(session->mutex);

  TrackedQuery q;
  q.time = fnord::WallClock::now();
  q.attrs = attrs;
  q.flushed = false;

  session->queries.emplace(eid, std::move(q));

  session->debugPrint(uid);
}

void LogJoinService::insertItemVisit(
    CustomerNamespace* customer,
    const fnord::DateTime& time,
    const std::string& uid,
    const std::string& eid,
    const ItemRef& item,
    std::vector<std::string> attrs) {
  auto session = findOrCreateSession(customer, uid);
  std::lock_guard<std::mutex> lock_holder(session->mutex);

}

TrackedSession* LogJoinService::findOrCreateSession(
    CustomerNamespace* customer,
    const std::string& uid) {
  TrackedSession* session = nullptr;

  {
    std::lock_guard<std::mutex> lock_holder(sessions_mutex_);

    auto siter = sessions_.find(uid);
    if (siter == sessions_.end()) {
      session = new TrackedSession();
      sessions_.emplace(uid, session);
    } else {
      session = siter->second.get();
    }
  }

  return session;
}

} // namespace cm
