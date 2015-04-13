/*
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <assert.h>
#include <fnord-base/exception.h>
#include <fnord-base/inspect.h>
#include <fnord-base/logging.h>
#include <fnord-base/stringutil.h>
#include <fnord-base/uri.h>
#include <fnord-base/wallclock.h>
#include <fnord-rpc/RPC.h>
#include <fnord-json/json.h>
#include <fnord-feeds/RemoteFeedFactory.h>
#include <fnord-feeds/RemoteFeedWriter.h>
#include "ItemRef.h"
#include "JoinedQuery.h"
#include "JoinedItemVisit.h"
#include "CustomerNamespace.h"
#include "logjoin/LogJoin.h"

using namespace fnord;

namespace cm {

LogJoin::LogJoin(
    LogJoinShard shard,
    bool dry_run,
    LogJoinTarget* target) :
    shard_(shard),
    dry_run_(dry_run),
    target_(target),
    sessions_flush_times_(1000000) {}

size_t LogJoin::numSessions() const {
  return sessions_flush_times_.size();
}

void LogJoin::insertLogline(
      const std::string& log_line,
      mdb::MDBTransaction* txn) {
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
  fnord::DateTime time(std::stoul(timestr) * fnord::kMicrosPerSecond);

  insertLogline(customer_key, time, body, txn);
}

void LogJoin::insertLogline(
    const std::string& customer_key,
    const fnord::DateTime& time,
    const std::string& log_line,
    mdb::MDBTransaction* txn) {
  fnord::URI::ParamList params;
  fnord::URI::parseQueryString(log_line, &params);

  try {
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

    if (!shard_.testUID(uid)) {
#ifndef NDEBUG
      fnord::logTrace(
          "cm.logjoin",
          "dropping logline with uid=$0 because it does not match my shard",
          uid);
#endif
      return;
    }

    stat_loglines_total_.incr(1);

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
        query.eid = eid;
        query.fromParams(params);
        insertQuery(customer_key, uid, query, txn);
        break;
      }

      /* item visit event */
      case 'v': {
        TrackedItemVisit visit;
        visit.time = time;
        visit.eid = eid;
        visit.fromParams(params);
        insertItemVisit(customer_key, uid, visit, txn);
        break;
      }

      case 'u':
        return;

      default:
        RAISE(kParseError, "invalid e param");

    }
  } catch (...) {
    stat_loglines_invalid_.incr(1);
    throw;
  }
}

void LogJoin::insertQuery(
    const std::string& customer_key,
    const std::string& uid,
    const TrackedQuery& query,
    mdb::MDBTransaction* txn) {
  withSession(customer_key, uid, txn, [this, &uid, &query] (TrackedSession* s) {
    bool merged = false;

    for (auto& q : s->queries) {
      if (q.eid == query.eid) {
        q.merge(query);
        merged = true;
        break;
      }
    }

    if (!merged) {
      s->queries.emplace_back(query);
    }

    if (query.time.unixMicros() > s->last_seen_unix_micros) {
      s->last_seen_unix_micros = query.time.unixMicros();
    }

    enqueueFlush(uid, s->nextFlushTime());
  });

}

void LogJoin::insertItemVisit(
    const std::string& customer_key,
    const std::string& uid,
    const TrackedItemVisit& visit,
    mdb::MDBTransaction* txn) {
  withSession(customer_key, uid, txn, [this, &visit, &uid] (TrackedSession* s) {
    bool merged = false;

    for (auto& v : s->item_visits) {
      if (v.eid == visit.eid) {
        v.merge(visit);
        merged = true;
        break;
      }
    }

    if (!merged) {
      s->item_visits.emplace_back(visit);
    }

    if (visit.time.unixMicros() > s->last_seen_unix_micros) {
      s->last_seen_unix_micros = visit.time.unixMicros();
    }

    enqueueFlush(uid, s->nextFlushTime());
  });
}

void LogJoin::withSession(
    const std::string& customer_key,
    const std::string& uid,
    mdb::MDBTransaction* txn,
    Function<void (TrackedSession* session)> fn) {
  TrackedSession session;

  auto cached = session_cache_.find(uid);
  if (cached == session_cache_.end()) {
    auto session_str = txn->get(uid);
    if (session_str.isEmpty()) {
      session.uid = uid;
      session.customer_key = customer_key;
      session.flushed = false;
      session.last_seen_unix_micros = 0;
    } else {
      session = json::fromJSON<TrackedSession>(session_str.get());
    }
  } else {
    session = cached->second;
  }

  fn(&session);

  if (session.flushed) {
    txn->del(uid);
    session_cache_.erase(uid);
  } else {
    auto serialized = json::toJSONString(session);
    txn->update(uid, serialized);
    session_cache_[uid] = session;
  }
}

void LogJoin::enqueueFlush(
    const String& uid,
    const DateTime& flush_at) {
  sessions_flush_times_[uid] = flush_at;
}

void LogJoin::flush(mdb::MDBTransaction* txn, DateTime stream_time_) {
  auto stream_time = stream_time_.unixMicros();

  for (auto iter = sessions_flush_times_.begin();
      iter != sessions_flush_times_.end();) {
    if (iter->second.unixMicros() < stream_time) {
      uint64_t next_flush = 0;
      const auto& uid = iter->first;

      withSession("", uid, txn, [this, &uid, &next_flush, &stream_time_, txn] (
          TrackedSession* s) {
        maybeFlushSession(txn, uid, s, stream_time_);

        if (!s->flushed) {
          next_flush = s->nextFlushTime().unixMicros();
        }
      });

      if (next_flush == 0) {
        iter = sessions_flush_times_.erase(iter);
        continue;
      } else {
        iter->second = DateTime(next_flush);
      }
    }

    ++iter;
  }
}

void LogJoin::maybeFlushSession(
    mdb::MDBTransaction* txn,
    const std::string uid,
    TrackedSession* session,
    DateTime stream_time_) {
  auto stream_time = stream_time_.unixMicros();

  /* flush queries */
  auto cur_query = session->queries.begin();
  while (cur_query != session->queries.end()) {
    if (stream_time > (cur_query->time.unixMicros() +
        kMaxQueryClickDelaySeconds * fnord::kMicrosPerSecond)) {

      /* search for matching item visits */
      auto cur_visit = session->item_visits.begin();
      while (cur_visit != session->item_visits.end()) {
        bool joined = false;

        if (cur_visit->time >= cur_query->time &&
            cur_visit->time.unixMicros() < (cur_query->time.unixMicros() +
                kMaxQueryClickDelaySeconds * fnord::kMicrosPerSecond)) {

          for (auto& qitem : cur_query->items) {
            if (cur_visit->item == qitem.item) {
              qitem.clicked = true;
              joined = true;
              target_->onItemVisit(txn, *session, *cur_visit, *cur_query);
              break;
            }
          }
        }

        if (joined) {
          cur_visit = session->item_visits.erase(cur_visit);
        } else {
          ++cur_visit;
        }
      }

      target_->onQuery(txn, *session, *cur_query);
      session->flushed_queries.emplace_back(*cur_query);
      cur_query = session->queries.erase(cur_query);
    } else {
      ++cur_query;
    }
  }

  /* flush item visits without query */
  auto cur_visit = session->item_visits.begin();
  while (cur_visit != session->item_visits.end()) {
    if (stream_time > (cur_visit->time.unixMicros() +
        kMaxQueryClickDelaySeconds * fnord::kMicrosPerSecond)) {
      target_->onItemVisit(txn, *session, *cur_visit);
      cur_visit = session->item_visits.erase(cur_visit);
    } else {
      ++cur_visit;
    }
  }

  /* flush session */
  if (stream_time > (session->last_seen_unix_micros +
      kSessionIdleTimeoutSeconds * fnord::kMicrosPerSecond)) {
    stat_joined_sessions_.incr(1);
    target_->onSession(txn, *session);
    session->flushed = true;
  }
}

void LogJoin::importTimeoutList(mdb::MDBTransaction* txn) {
  Buffer key;
  Buffer value;
  int n = 0;

  auto cursor = txn->getCursor();

  for (;;) {
    bool eof;
    if (n++ == 0) {
      eof = !cursor->getFirst(&key, &value);
    } else {
      eof = !cursor->getNext(&key, &value);
    }

    if (eof) {
      break;
    }

    if (StringUtil::beginsWith(key.toString(), "__")) {
      continue;
    }

    auto session = json::fromJSON<TrackedSession>(value);
    enqueueFlush(session.uid, session.nextFlushTime());
  }

  cursor->close();
}

void LogJoin::exportStats(const std::string& prefix) {
  exportStat(
      StringUtil::format("$0/$1", prefix, "loglines_total"),
      &stat_loglines_total_,
      fnord::stats::ExportMode::EXPORT_DELTA);

  exportStat(
      StringUtil::format("$0/$1", prefix, "loglines_invalid"),
      &stat_loglines_invalid_,
      fnord::stats::ExportMode::EXPORT_DELTA);

  exportStat(
      StringUtil::format("$0/$1", prefix, "joined_sessions"),
      &stat_joined_sessions_,
      fnord::stats::ExportMode::EXPORT_DELTA);

  exportStat(
      StringUtil::format("$0/$1", prefix, "joined_queries"),
      &stat_joined_queries_,
      fnord::stats::ExportMode::EXPORT_DELTA);

  exportStat(
      StringUtil::format("$0/$1", prefix, "joined_item_visits"),
      &stat_joined_item_visits_,
      fnord::stats::ExportMode::EXPORT_DELTA);
}

} // namespace cm
