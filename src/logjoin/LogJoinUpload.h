/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_LOGJOINUPLOAD_H
#define _CM_LOGJOINUPLOAD_H
#include "stx/stdtypes.h"
#include "stx/mdb/MDB.h"
#include "stx/thread/FixedSizeThreadPool.h"
#include "stx/protobuf/MessageSchema.h"
#include "stx/protobuf/msg.h"
#include "stx/protobuf/JSONEncoder.h"
#include "stx/http/httpconnectionpool.h"
#include "brokerd/BrokerClient.h"
#include "logjoin/JoinedSession.pb.h"
#include "common/CustomerDirectory.h"

using namespace fnord;

namespace cm {

class LogJoinUpload {
public:
  static const size_t kDefaultBatchSize = 10;

  LogJoinUpload(
      CustomerDirectory* customer_dir,
      RefPtr<mdb::MDB> db,
      const String& tsdb_addr,
      const String& brokerd_addr,
      http::HTTPConnectionPool* http);

  void upload();

  void start();
  void stop();

protected:

  void onSession(const JoinedSession& session);

  void deliverWebhooks(
      const LogJoinConfig& conf,
      const JoinedSession& session);

  size_t scanQueue(const String& queue_name);
  void uploadTSDBBatch(const Vector<Buffer>& batch);

  CustomerDirectory* customer_dir_;
  RefPtr<mdb::MDB> db_;
  String tsdb_addr_;
  InetAddr broker_addr_;
  http::HTTPConnectionPool* http_;
  feeds::BrokerClient broker_client_;
  size_t batch_size_;
  thread::FixedSizeThreadPool webhook_delivery_tpool_;
};

} // namespace cm
#endif
