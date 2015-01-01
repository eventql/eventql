/*
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <assert.h>
#include <fnord/base/exception.h>
#include <fnord/base/inspect.h>
#include <fnord/base/stringutil.h>
#include <fnord/base/uri.h>
#include <fnord/base/wallclock.h>
#include <fnord/comm/rpc.h>
#include <fnord/json/json.h>
#include <fnord/service/logstream/logstreamservice.h>
#include "itemref.h"
#include "customernamespace.h"
#include "logjoin.h"

using fnord::logstream_service::LogStreamService;

namespace cm {

LogJoin::LogJoin(
    fnord::thread::TaskScheduler* scheduler) :
    scheduler_(scheduler),
    stream_clock_(0) {
}

void LogJoin::insertLogline(const std::string& log_line) {
  auto c_end = log_line.find("|");
  if (c_end == std::string::npos) {
    RAISEF(kRuntimeError, "invalid logline: $0", log_line);
  }

  auto t_end = log_line.find("|", c_end + 1);
  if (t_end == std::string::npos) {
    RAISEF(kRuntimeError, "invalid logline: $0", log_line);
  }

  auto customer_key = log_line.substr(0, c_end);
  auto body = log_line.substr(t_end + 1);
  auto timestr = log_line.substr(c_end + 1, t_end - c_end - 1);
  fnord::DateTime time(std::stoul(timestr) * fnord::DateTime::kMicrosPerSecond);

  insertLogline(customer_key, time, body);
}

void LogJoin::insertLogline(
    const std::string& customer_key,
    const fnord::DateTime& time,
    const std::string& log_line) {
  auto stream_time = streamTime(time);

  fnord::URI::ParamList params;
  fnord::URI::parseQueryString(log_line, &params);

  flush(stream_time);

  /* extract uid (userid) and eid (eventid) */
  std::string c;
  if (!fnord::URI::getParam(params, "c", &c)) {
    RAISE(kParseError, "c param is missing");
  }

  auto c_s = c.find("~");
  if (c_s == std::string::npos) {
    RAISE(kParseError, "c param is invalid");
  }

  std::string uid = c.substr(0, c_s);
  std::string eid = c.substr(c_s + 1, c.length() - c_s - 1);
  if (uid.length() == 0 || eid.length() == 0) {
    RAISE(kParseError, "c param is invalid");
  }

  /* extract the event type */
  std::string evtype;
  if (!fnord::URI::getParam(params, "e", &evtype) || evtype.length() != 1) {
    RAISE(kParseError, "e param is missing");
  }

  /* process event */
  switch (evtype[0]) {

    /* query event */
    case 'q': {
      TrackedQuery query;
      query.time = time;
      query.flushed = false;
      query.fromParams(params);
      insertQuery(customer_key, uid, eid, query);
      break;
    }

    /* item visit event */
    case 'v': {
      TrackedItemVisit visit;
      visit.time = time;
      visit.fromParams(params);
      insertItemVisit(customer_key, uid, eid, visit);
      break;
    }

    case 'u':
      return;

    default:
      RAISE(kParseError, "invalid e param");

  }
}

void LogJoin::insertQuery(
    const std::string& customer_key,
    const std::string& uid,
    const std::string& eid,
    const TrackedQuery& query) {
  auto session = findOrCreateSession(customer_key, uid);
  std::lock_guard<std::mutex> l(session->mutex, std::adopt_lock_t {});

  auto iter = session->queries.find(eid);
  if (iter == session->queries.end()) {
    session->queries.emplace(eid, query);
  } else {
    iter->second.merge(query);
  }

  if (query.time.unixMicros() > session->last_seen_unix_micros) {
    session->last_seen_unix_micros = query.time.unixMicros();
  }
}

void LogJoin::insertItemVisit(
    const std::string& customer_key,
    const std::string& uid,
    const std::string& eid,
    const TrackedItemVisit& visit) {
  {
    auto session = findOrCreateSession(customer_key, uid);
    std::lock_guard<std::mutex> l(session->mutex, std::adopt_lock_t {});

    auto iter = session->item_visits.find(eid);
    if (iter == session->item_visits.end()) {
      session->item_visits.emplace(eid, visit);
    } else {
      iter->second.merge(visit);
    }

    if (visit.time.unixMicros() > session->last_seen_unix_micros) {
      session->last_seen_unix_micros = visit.time.unixMicros();
    }
  }

  recordJoinedItemVisit(customer_key, uid, eid, visit);
}

bool LogJoin::maybeFlushSession(
    const std::string uid,
    TrackedSession* session,
    const fnord::DateTime& stream_time) {
  if (static_cast<uint64_t>(stream_time) < session->last_seen_unix_micros) {
    return false;
  }

  auto tdiff =
      static_cast<uint64_t>(stream_time) -
      static_cast<uint64_t>(session->last_seen_unix_micros);
  bool do_flush =
      tdiff > kSessionIdleTimeoutSeconds * fnord::DateTime::kMicrosPerSecond;
  bool do_update = do_flush;

  std::vector<std::pair<std::string, TrackedQuery*>> flushed;
  for (auto& query_pair : session->queries) {
    const auto& eid = query_pair.first;
    auto& query = query_pair.second;

    if (!query.flushed && (
            static_cast<uint64_t>(stream_time) -
            static_cast<uint64_t>(query.time)) >
            kMaxQueryClickDelaySeconds * fnord::DateTime::kMicrosPerSecond) {
      query.flushed = true;
      do_update = true;
      flushed.emplace_back(eid, &query);
    }
  }

  if (do_update) {
    session->update();
  }

  for (const auto flushed_query : flushed) {
    recordJoinedQuery(
        session->customer_key,
        uid,
        flushed_query.first,
        *flushed_query.second);
  }

  if (do_flush) {
    recordJoinedSession(session->customer_key, uid, *session);
  }

  return do_flush;
}

void LogJoin::flush(const fnord::DateTime& stream_time) {
  std::lock_guard<std::mutex> l1(sessions_mutex_);

  for (auto iter = sessions_.begin(); iter != sessions_.end(); ) {
    const auto& uid = iter->first;
    const auto& session = iter->second;

    std::lock_guard<std::mutex> l2(session->mutex);
    if (maybeFlushSession(uid, session.get(), stream_time)) {
      iter = sessions_.erase(iter);
    } else {
      ++iter;
    }
  }
}

TrackedSession* LogJoin::findOrCreateSession(
    const std::string& customer_key,
    const std::string& uid) {
  TrackedSession* session = nullptr;

  std::lock_guard<std::mutex> lock_holder(sessions_mutex_);

  auto siter = sessions_.find(uid);
  if (siter == sessions_.end()) {
    session = new TrackedSession();
    session->customer_key = customer_key;
    session->last_seen_unix_micros = 0;
    sessions_.emplace(uid, std::unique_ptr<TrackedSession>(session));
  } else {
    session = siter->second.get();
  }

  session->mutex.lock();
  return session;
}

fnord::DateTime LogJoin::streamTime(const fnord::DateTime& time) {
  std::lock_guard<std::mutex> l(stream_clock_mutex_);

  if (time > stream_clock_) {
    stream_clock_ = time;
  }

  return stream_clock_;
}

void LogJoin::recordJoinedQuery(
    const std::string& customer_key,
    const std::string& uid,
    const std::string& eid,
    const TrackedQuery& query) {
}

void LogJoin::recordJoinedItemVisit(
    const std::string& customer_key,
    const std::string& uid,
    const std::string& eid,
    const TrackedItemVisit& visit) {
}

void LogJoin::recordJoinedSession(
    const std::string& customer_key,
    const std::string& uid,
    const TrackedSession& session) {
  fnord::iputs("stream_time=$0 active_sessions=$1", stream_clock_, sessions_.size());
}

} // namespace cm
