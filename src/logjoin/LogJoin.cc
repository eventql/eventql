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
    bool enable_cache,
    LogJoinTarget* target) :
    shard_(shard),
    dry_run_(dry_run),
    target_(target),
    turbo_(false),
    enable_cache_(enable_cache) {
  addPixelParamID("dw_ab", 1);
  addPixelParamID("l", 2);
  addPixelParamID("u_x", 3);
  addPixelParamID("u_y", 4);
  addPixelParamID("is", 5);
  addPixelParamID("pg", 6);
  addPixelParamID("q_cat1", 7);
  addPixelParamID("q_cat2", 8);
  addPixelParamID("q_cat3", 9);
  addPixelParamID("slrid", 10);
  addPixelParamID("i", 11);
  addPixelParamID("s", 12);
  addPixelParamID("ml", 13);
  addPixelParamID("adm", 14);
  addPixelParamID("lgn", 15);
  addPixelParamID("slr", 16);
  addPixelParamID("lng", 17);
  addPixelParamID("dwnid", 18);
  addPixelParamID("fnm", 19);
  addPixelParamID("qstr~de", 100);
  addPixelParamID("qstr~pl", 101);
  addPixelParamID("qstr~en", 102);
  addPixelParamID("qstr~fr", 103);
  addPixelParamID("qstr~it", 104);
  addPixelParamID("qstr~nl", 105);
  addPixelParamID("qstr~es", 106);
}

size_t LogJoin::numSessions() const {
  return sessions_flush_times_.size();
}

size_t LogJoin::cacheSize() const {
  return session_cache_.size();
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

  stat_loglines_total_.incr(1);

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

    std::string evtype;
    if (!fnord::URI::getParam(params, "e", &evtype) || evtype.length() != 1) {
      RAISE(kParseError, "e param is missing");
    }

    if (evtype.length() != 1) {
      RAISE(kParseError, "e param invalid");
    }

    /* process event */
    switch (evtype[0]) {

      case 'q':
      case 'v':
      case 'c':
      case 'u':
        break;

      default:
        RAISE(kParseError, "invalid e param");

    };

    URI::ParamList stored_params;
    for (const auto& p : params) {
      if (p.first == "c" || p.first == "e" || p.first == "v") {
        continue;
      }

      stored_params.emplace_back(p);
    }

    appendToSession(customer_key, time, uid, eid, evtype, stored_params, txn);
  } catch (...) {
    stat_loglines_invalid_.incr(1);
    throw;
  }
}

void LogJoin::appendToSession(
    const std::string& customer_key,
    const fnord::DateTime& time,
    const std::string& uid,
    const std::string& evid,
    const std::string& evtype,
    const Vector<Pair<String, String>>& logline,
    mdb::MDBTransaction* txn) {

  auto flush_at = time.unixMicros() +
      kSessionIdleTimeoutSeconds * fnord::kMicrosPerSecond;

  auto old_ftime = sessions_flush_times_.find(uid);
  if (old_ftime == sessions_flush_times_.end() ||
      old_ftime->second < flush_at) {
    sessions_flush_times_.emplace(uid, flush_at);
  }

  util::BinaryMessageWriter buf;
  buf.appendVarUInt(time.unixMicros() / kMicrosPerSecond);
  buf.appendVarUInt(evid.length());
  buf.append(evid.data(), evid.length());
  for (const auto& p : logline) {
    buf.appendVarUInt(getPixelParamID(p.first));
    buf.appendVarUInt(p.second.size());
    buf.append(p.second.data(), p.second.size());
  }

  auto evkey = uid + "~" + evtype + "~" + rnd_.hex64();
  txn->insert(evkey.data(), evkey.size(), buf.data(), buf.size());
  txn->update(uid + "~cust", customer_key);
}

void LogJoin::flush(mdb::MDBTransaction* txn, DateTime stream_time_) {
  auto stream_time = stream_time_.unixMicros();

  for (auto iter = sessions_flush_times_.begin();
      iter != sessions_flush_times_.end();) {
    if (iter->second.unixMicros() < stream_time) {
      flushSession(iter->first, stream_time, txn);
      iter = sessions_flush_times_.erase(iter);
    } else {
      ++iter;
    }
  }
}

void LogJoin::flushSession(
    const std::string uid,
    DateTime stream_time,
    mdb::MDBTransaction* txn) {
  auto cursor = txn->getCursor();

  TrackedSession session;
  session.uid = uid;

  Buffer key;
  Buffer value;
  bool eof = false;
  for (int i = 0; ; ++i) {
    if (i == 0) {
      key.append(uid);
      if (!cursor->getFirstOrGreater(&key, &value)) {
        break;
      }
    } else {
      if (!cursor->getNext(&key, &value)) {
        break;
      }
    }

    auto key_str = key.toString();
    if (!StringUtil::beginsWith(key_str, uid)) {
      break;
    }

    if (StringUtil::endsWith(key_str, "~cust")) {
      session.customer_key = value.toString();
    } else {
      auto evtype = key_str.substr(uid.length() + 1).substr(0, 1);

      util::BinaryMessageReader reader(value.data(), value.size());
      auto time = reader.readVarUInt() * kMicrosPerSecond;
      auto evid_len = reader.readVarUInt();
      auto evid = String((char*) reader.read(evid_len), evid_len);

      try {
        URI::ParamList logline;
        while (reader.remaining() > 0) {
          auto key = getPixelParamName(reader.readVarUInt());
          auto len = reader.readVarUInt();
          logline.emplace_back(key, String((char*) reader.read(len), len));
        }

        session.insertLogline(time, evtype, evid, logline);
      } catch (const std::exception& e) {
        fnord::logError("cm.logjoin", e, "invalid logline");
        stat_loglines_invalid_.incr(1);
      }
    }

    cursor->del(key);
  }

  cursor->close();

  if (session.customer_key.length() == 0) {
    fnord::logError("cm.logjoin", "missing customer key for: $0", uid);
    return;
  }

  try {
    target_->onSession(txn, session);
  } catch (const std::exception& e) {
    fnord::logError("cm.logjoin", e, "LogJoinTarget::onSession crashed");
  }

  stat_joined_sessions_.incr(1);
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

    auto sid = key.toString();
    if (StringUtil::endsWith(sid, "~cust")) {
      continue;
    }

    sid.erase(sid.begin() + sid.find("~"), sid.end());

    util::BinaryMessageReader reader(value.data(), value.size());
    auto time = reader.readVarUInt();
    auto ftime = (time + kSessionIdleTimeoutSeconds) * fnord::kMicrosPerSecond;

    auto old_ftime = sessions_flush_times_.find(sid);
    if (old_ftime == sessions_flush_times_.end() ||
        old_ftime->second.unixMicros() < ftime) {
      sessions_flush_times_.emplace(sid, ftime);
    }
  }

  cursor->close();
}

void LogJoin::addPixelParamID(const String& param, uint32_t id) {
  pixel_param_ids_[param] = id;
  pixel_param_names_[id] = param;
}

uint32_t LogJoin::getPixelParamID(const String& param) const {
  auto p = pixel_param_ids_.find(param);

  if (p == pixel_param_ids_.end()) {
    RAISEF(kIndexError, "invalid pixel param: $0", param);
  }

  return p->second;
}

const String& LogJoin::getPixelParamName(uint32_t id) const {
  auto p = pixel_param_names_.find(id);

  if (p == pixel_param_names_.end()) {
    RAISEF(kIndexError, "invalid pixel param: $0", id);
  }

  return p->second;
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

void LogJoin::setTurbo(bool turbo) {
  turbo_ = turbo;
}

} // namespace cm
