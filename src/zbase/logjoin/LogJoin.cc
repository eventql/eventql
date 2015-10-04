/*
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stx/exception.h>
#include <stx/inspect.h>
#include <stx/logging.h>
#include <stx/stringutil.h>
#include <stx/uri.h>
#include <stx/wallclock.h>
#include <stx/rpc/RPC.h>
#include <stx/json/json.h>
#include <stx/protobuf/msg.h>
#include "stx/util/binarymessagereader.h"
#include "stx/util/binarymessagewriter.h"
#include <brokerd/RemoteFeedFactory.h>
#include <brokerd/RemoteFeedWriter.h>
#include <zbase/docdb/ItemRef.h>
#include "zbase/logjoin/LogJoin.h"

using namespace stx;

namespace zbase {

LogJoin::LogJoin(
    LogJoinShard shard,
    bool dry_run,
    RefPtr<mdb::MDB> sessdb,
    SessionProcessor* target,
    thread::EventLoop* evloop) :
    shard_(shard),
    dry_run_(dry_run),
    sessdb_(sessdb),
    target_(target),
    shutdown_(false),
    rpc_client_(evloop),
    feed_reader_(&rpc_client_) {
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
  addPixelParamID("r_url", 20);
  addPixelParamID("r_nm", 21);
  addPixelParamID("r_cpn", 22);
  addPixelParamID("x", 23);
  addPixelParamID("qx", 24);
  addPixelParamID("cs", 25);
  addPixelParamID("qt", 26);
  addPixelParamID("qstr~de", 100);
  addPixelParamID("qstr~pl", 101);
  addPixelParamID("qstr~en", 102);
  addPixelParamID("qstr~fr", 103);
  addPixelParamID("qstr~it", 104);
  addPixelParamID("qstr~nl", 105);
  addPixelParamID("qstr~es", 106);
  addPixelParamID("__evdata", 200);

  feed_reader_.setMaxSpread(10 * kMicrosPerSecond);
  feed_reader_.setTimeBackfill([] (const feeds::FeedEntry& entry) -> UnixTime {
    const auto& log_line = entry.data;

    auto c_end = log_line.find("|");
    if (c_end == std::string::npos) return 0;
    auto t_end = log_line.find("|", c_end + 1);
    if (t_end == std::string::npos) return 0;

    auto timestr = log_line.substr(c_end + 1, t_end - c_end - 1);
    return std::stoul(timestr) * stx::kMicrosPerSecond;
  });
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
  stx::UnixTime time(std::stoul(timestr) * stx::kMicrosPerSecond);

  if (StringUtil::beginsWith(body, "1|")) {
    insertLogline(customer_key, time, body.substr(2), txn);
  } else {
    insertLegacyLogline(customer_key, time, body, txn);
  }
}

void LogJoin::insertLogline(
    const std::string& customer_key,
    const stx::UnixTime& time,
    const std::string& log_line,
    mdb::MDBTransaction* txn) {

  auto uid_end = log_line.find("~");
  if (uid_end == std::string::npos) {
    RAISEF(kRuntimeError, "invalid logline: $0", log_line);
  }

  auto evid_end = log_line.find("~", uid_end + 1);
  if (evid_end == std::string::npos) {
    RAISEF(kRuntimeError, "invalid logline: $0", log_line);
  }

  auto evtype_end = log_line.find("~", evid_end + 1);
  if (evtype_end == std::string::npos) {
    RAISEF(kRuntimeError, "invalid logline: $0", log_line);
  }

  auto uid = log_line.substr(0, uid_end);
  auto evid = log_line.substr(uid_end + 1, evid_end - uid_end - 1);
  auto evtype = log_line.substr(evid_end + 1, evtype_end - evid_end - 1);
  auto evdata = log_line.substr(evtype_end + 1);

  if (!shard_.testUID(uid)) {
#ifndef NDEBUG
    stx::logTrace(
        "logjoind",
        "dropping logline with uid=$0 because it does not match my shard",
        uid);
#endif
    return;
  }

  appendToSession(customer_key, time, uid, evid, evtype, evdata, txn);
}

void LogJoin::insertLegacyLogline(
    const std::string& customer_key,
    const stx::UnixTime& time,
    const std::string& log_line,
    mdb::MDBTransaction* txn) {
  stx::URI::ParamList params;
  stx::URI::parseQueryString(log_line, &params);

  stat_loglines_total_.incr(1);

  try {
    /* extract uid (userid) and eid (eventid) */
    std::string c;
    if (!stx::URI::getParam(params, "c", &c)) {
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
      stx::logTrace(
          "logjoind",
          "dropping logline with uid=$0 because it does not match my shard",
          uid);
#endif
      return;
    }

    std::string evtype;
    if (!stx::URI::getParam(params, "e", &evtype) || evtype.length() != 1) {
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
    const stx::UnixTime& time,
    const std::string& uid,
    const std::string& evid,
    const std::string& evtype,
    const String& evdata,
    mdb::MDBTransaction* txn) {
  Vector<Pair<String, String>> logline;
  logline.emplace_back("__evdata",  evdata);

  appendToSession(
      customer_key,
      time,
      uid,
      evid,
      evtype,
      logline,
      txn);
}

void LogJoin::appendToSession(
    const std::string& customer_key,
    const stx::UnixTime& time,
    const std::string& uid,
    const std::string& evid,
    const std::string& evtype,
    const Vector<Pair<String, String>>& logline,
    mdb::MDBTransaction* txn) {

  auto flush_at = time.unixMicros() +
      kSessionIdleTimeoutSeconds * stx::kMicrosPerSecond;

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

void LogJoin::flush(mdb::MDBTransaction* txn, UnixTime stream_time_) {
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
    UnixTime stream_time,
    mdb::MDBTransaction* txn) {
  TrackedSession session;
  session.uuid = SHA1::compute(uid).toString();

  auto cursor = txn->getCursor();
  try {

    Buffer key;
    Buffer value;
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
        auto evtype = key_str.substr(uid.length() + 1);
        auto evtype_end = evtype.find("~");
        if (evtype_end != String::npos) {
          evtype = evtype.substr(0, evtype_end);
        }

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
          stx::logError("logjoind", e, "invalid logline");
          stat_loglines_invalid_.incr(1);
        }
      }

      cursor->del();
    }

    cursor->close();
  } catch (StandardException& e) {
    cursor->close();
    logError("logjond", e, "error while processing session");
  }

  if (session.customer_key.length() == 0) {
    stx::logError("logjoind", "missing customer key for: $0", uid);
    return;
  }

  try {
    target_->enqueueSession(session);
  } catch (const std::exception& e) {
    stx::logError("logjoind", e, "SessionProcessor::enqueueSession crashed");
  }

  stat_joined_sessions_.incr(1);
}

//void LogJoin::onSession(
//    mdb::MDBTransaction* txn,
//    TrackedSession& session) {
//  auto session_data = target_->joinSession(session);
//
//  if (dry_run_) {
//    stx::logInfo(
//        "logjoind",
//        "[DRYRUN] not uploading session: ", 1);
//        //msg::MessagePrinter::print(obj, joined_session_schema_));
//    return;
//  }
//
//  SessionEnvelope envelope;
//  envelope.set_customer(session.customer_key);
//  envelope.set_session_id(session.uid);
//  envelope.set_time(session.firstSeenTime().get().unixMicros());
//  envelope.set_session_data(session_data.toString());
//
//  auto envelope_buf = msg::encode(envelope);
//  auto key = StringUtil::format("__sessionq-$0",  rnd_.hex128());
//  txn->update(
//      key.data(),
//      key.size(),
//      envelope_buf->data(),
//      envelope_buf->size());
//}

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
    auto ftime = (time + kSessionIdleTimeoutSeconds) * stx::kMicrosPerSecond;

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
      stx::stats::ExportMode::EXPORT_DELTA);

  exportStat(
      StringUtil::format("$0/$1", prefix, "loglines_invalid"),
      &stat_loglines_invalid_,
      stx::stats::ExportMode::EXPORT_DELTA);

  exportStat(
      StringUtil::format("$0/$1", prefix, "joined_sessions"),
      &stat_joined_sessions_,
      stx::stats::ExportMode::EXPORT_DELTA);

  exportStat(
      StringUtil::format("$0/$1", prefix, "joined_queries"),
      &stat_joined_queries_,
      stx::stats::ExportMode::EXPORT_DELTA);

  exportStat(
      StringUtil::format("$0/$1", prefix, "joined_item_visits"),
      &stat_joined_item_visits_,
      stx::stats::ExportMode::EXPORT_DELTA);

  exportStat(
      StringUtil::format("/logjoind/$0/stream_time_low", shard_.shard_name),
      &stat_stream_time_low,
      stx::stats::ExportMode::EXPORT_VALUE);

  exportStat(
      StringUtil::format("/logjoind/$0/stream_time_high", shard_.shard_name),
      &stat_stream_time_high,
      stx::stats::ExportMode::EXPORT_VALUE);

  exportStat(
      StringUtil::format("/logjoind/$0/active_sessions", shard_.shard_name),
      &stat_active_sessions,
      stx::stats::ExportMode::EXPORT_VALUE);

  exportStat(
      StringUtil::format("/logjoind/$0/dbsize", shard_.shard_name),
      &stat_dbsize,
      stx::stats::ExportMode::EXPORT_VALUE);

}

void LogJoin::processClickstream(
      const HashMap<String, URI>& input_feeds,
      size_t batch_size,
      size_t buffer_size,
      Duration flush_interval) {
  /* resume from last offset */
  {
    auto txn = sessdb_->startTransaction(true);
    try {
      importTimeoutList(txn.get());

      for (const auto& input_feed : input_feeds) {
        uint64_t offset = 0;

        if (input_feed.first == "tracker_log.feedserver03.production.fnrd.net") {
          offset = 86325404806llu;
        }

        if (input_feed.first == "tracker_log.feedserver02.nue01.production.fnrd.net") {
          offset = 275512545617llu;
        }

        auto last_offset = txn->get(
            StringUtil::format("__logjoin_offset~$0", input_feed.first));

        if (!last_offset.isEmpty()) {
          offset = std::stoul(last_offset.get().toString());
        }

        stx::logInfo(
            "logjoind",
            "Adding source feed:\n    feed=$0\n    url=$1\n    offset: $2",
            input_feed.first,
            input_feed.second.toString(),
            offset);

        feed_reader_.addSourceFeed(
            input_feed.second,
            input_feed.first,
            offset,
            batch_size,
            buffer_size);
      }

      txn->abort();
    } catch (...) {
      txn->abort();
      throw;
    }
  }

  stx::logInfo("logjoind", "Resuming logjoin...");

  UnixTime last_flush;
  uint64_t rate_limit_micros = flush_interval.microseconds();

  for (;;) {
    auto begin = WallClock::unixMicros();

    feed_reader_.fillBuffers();
    auto txn = sessdb_->startTransaction();

    int i = 0;
    for (; i < batch_size; ++i) {
      auto entry = feed_reader_.fetchNextEntry();

      if (entry.isEmpty()) {
        break;
      }

      try {
        insertLogline(entry.get().data, txn.get());
      } catch (const std::exception& e) {
        stx::logError("logjoind", e, "invalid log line");
      }
    }

    auto watermarks = feed_reader_.watermarks();

    auto etime = WallClock::now().unixMicros() - last_flush.unixMicros();
    if (etime > rate_limit_micros) {
      last_flush = WallClock::now();
      flush(txn.get(), watermarks.first);
    }

    auto stream_offsets = feed_reader_.streamOffsets();
    String stream_offsets_str;

    for (const auto& soff : stream_offsets) {
      txn->update(
          StringUtil::format("__logjoin_offset~$0", soff.first),
          StringUtil::toString(soff.second));

      stream_offsets_str +=
          StringUtil::format("\n    offset[$0]=$1", soff.first, soff.second);
    }

    stx::logInfo(
        "logjoind",
        "LogJoin comitting...\n    stream_time=<$0 ... $1>\n" \
        "    active_sessions=$2\n    flushed_sessions=$3$4",
        watermarks.first,
        watermarks.second,
        numSessions(),
        0, //session_proc.num_sessions,
        stream_offsets_str);

    if (dry_run_) {
      txn->abort();
    } else {
      txn->commit();
    }

    //sessdb_->removeStaleReaders();

    if (shutdown_.load()) {
      break;
    }

    stat_stream_time_low.set(watermarks.first.unixMicros());
    stat_stream_time_high.set(watermarks.second.unixMicros());
    stat_active_sessions.set(numSessions());
    //stat_dbsize.set(FileUtil::du_c(flags.getString("datadir"));

    auto rtime = WallClock::unixMicros() - begin;
    auto rlimit = kMicrosPerSecond;
    if (i < 2 && rtime < rlimit) {
      usleep(rlimit - rtime);
    }
  }
}

void LogJoin::shutdown() {
  shutdown_ = true;
}

} // namespace zbase
