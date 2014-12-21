/*
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <fnord/base/inspect.h>
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
  insertLogline(customer, fnord::DateTime(), log_line);
}

void LogJoinService::insertLogline(
    CustomerNamespace* customer,
    const fnord::DateTime& time,
    const std::string& log_line) {
  fnord::URI::ParamList params;
  fnord::URI::parseQueryString(log_line, &params);

  /* extract uid (userid) and eid (eventid) */
  std::string c;
  if (!fnord::URI::getParam(params, "c", &c)) {
    return;
  }

  auto c_s = c.find("~");
  if (c_s == std::string::npos) {
    return;
  }

  std::string uid = c.substr(0, c_s);
  std::string eid = c.substr(c_s + 1, c.length() - c_s - 1);
  if (uid.length() == 0 || eid.length() == 0) {
    return;
  }

  /* extract the event type */
  std::string evtype;
  if (!fnord::URI::getParam(params, "e", &evtype) || evtype.length() != 1) {
    return;
  }

  /* extract all non-reserved params as event attributes */
  std::vector<std::string> attrs;
  for (const auto& p : params) {
    if (p.first != "c" && p.first != "e" && p.first != "i" && p.first != "is") {
      attrs.emplace_back(fnord::StringUtil::format("$0:$1", p.first, p.second));
    }
  }

  /* process event */
  switch (evtype[0]) {

    /* query event */
    case 'q': {
      TrackedQuery q;
      q.time = time;
      q.attrs = attrs;
      q.flushed = false;

      std::string items_str;
      if (fnord::URI::getParam(params, "is", &items_str)) {
        for (const auto& item_str : fnord::StringUtil::split(items_str, ",")) {
          auto item_str_parts = fnord::StringUtil::split(item_str, "~");
          if (item_str_parts.size() < 2) {
            return;
          }

          TrackedQueryItem qitem;
          qitem.item.set_id = item_str_parts[0];
          qitem.item.item_id = item_str_parts[1];
          qitem.clicked = false;
          qitem.position = -1;
          qitem.variant = -1;

          for (int i = 2; i < item_str_parts.size(); ++i) {
            const auto& iattr = item_str_parts[i];
            if (iattr.length() < 1) {
              continue;
            }

            switch (iattr[0]) {
              case 'p':
                qitem.position = std::stoi(iattr.substr(1, iattr.length() - 1));
                break;
              case 'v':
                qitem.variant = std::stoi(iattr.substr(1, iattr.length() - 1));
                break;
            }
          }

          q.items.emplace_back(qitem);
        }
      }

      insertQuery(customer, uid, eid, q);
      break;
    }

    /* item visit event */
    case 'v': {
      std::string item_id_str;
      if (!fnord::URI::getParam(params, "i", &item_id_str)) {
        return;
      }

      auto item_id_parts = fnord::StringUtil::split(item_id_str, "~");
      if (item_id_parts.size() < 2) {
        return;
      }

      TrackedItemVisit visit;
      visit.time = time;
      visit.item.set_id = item_id_parts[0];
      visit.item.item_id = item_id_parts[1];
      visit.attrs = attrs;

      insertItemVisit(
          customer,
          uid,
          eid,
          visit);

      break;
    }

  }
}
void LogJoinService::insertQuery(
    CustomerNamespace* customer,
    const std::string& uid,
    const std::string& eid,
    const TrackedQuery& query) {
  auto session = findOrCreateSession(customer, uid);
  std::lock_guard<std::mutex> lock_holder(session->mutex);

  auto iter = session->queries.find(eid);
  if (iter == session->queries.end()) {
    session->queries.emplace(eid, query);
  } else {
    iter->second.merge(query);
  }

  if (query.time.unixMicros() > session->last_seen_unix_micros) {
    session->last_seen_unix_micros = query.time.unixMicros();
  }

  session->debugPrint(uid);
}

void LogJoinService::insertItemVisit(
    CustomerNamespace* customer,
    const std::string& uid,
    const std::string& eid,
    const TrackedItemVisit& visit) {
  auto session = findOrCreateSession(customer, uid);
  std::lock_guard<std::mutex> lock_holder(session->mutex);

  auto iter = session->item_visits.find(eid);
  if (iter == session->item_visits.end()) {
    session->item_visits.emplace(eid, visit);
  } else {
    iter->second.merge(visit);
  }

  if (visit.time.unixMicros() > session->last_seen_unix_micros) {
    session->last_seen_unix_micros = visit.time.unixMicros();
  }

  session->debugPrint(uid);
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
