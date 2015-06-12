/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_INDEXFEEDUPLOAD_H
#define _CM_INDEXFEEDUPLOAD_H
#include "fnord/stdtypes.h"
#include "fnord/thread/queue.h"
#include "fnord/http/httpconnectionpool.h"
#include "fnord/protobuf/MessageSchema.h"
#include "IndexChangeRequest.h"
#include <thread>

using namespace fnord;

namespace cm {

class IndexFeedUpload {
public:
  static const size_t kDefaultBatchSize = 10;

  IndexFeedUpload(
      const String& target_url,
      thread::Queue<IndexChangeRequest>* queue,
      http::HTTPConnectionPool* http,
      RefPtr<msg::MessageSchema> schema);

  void start();
  void uploadNext();
  void stop();

protected:

  void run();

  void uploadWithRetries(const http::HTTPRequest& req);
  void uploadBatch(
      const String& customer,
      const Vector<IndexChangeRequest>& batch);

  String target_url_;
  thread::Queue<IndexChangeRequest>* queue_;
  http::HTTPConnectionPool* http_;
  size_t batch_size_;
  bool running_;
  std::thread thread_;
  RefPtr<msg::MessageSchema> schema_;
};
} // namespace cm

#endif
