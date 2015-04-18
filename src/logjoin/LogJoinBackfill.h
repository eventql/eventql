/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_LOGJOINBACKFILL_H
#define _CM_LOGJOINBACKFILL_H
#include "fnord-base/io/filerepository.h"
#include "fnord-base/io/fileutil.h"
#include "fnord-base/application.h"
#include "fnord-base/logging.h"
#include "fnord-base/random.h"
#include "fnord-base/uri.h"
#include "fnord-base/thread/eventloop.h"
#include "fnord-base/thread/threadpool.h"
#include "fnord-base/thread/queue.h"
#include "fnord-base/wallclock.h"
#include "fnord-base/cli/flagparser.h"
#include "fnord-eventdb/TableRepository.h"
#include "fnord-eventdb/LogTableTail.h"
#include "fnord-msg/MessageEncoder.h"
#include "fnord-http/httpconnectionpool.h"

using namespace fnord;

namespace cm {

class LogJoinBackfill {
public:
  typedef Function<void (msg::MessageObject* record)> BackfillFnType;

  LogJoinBackfill(
      RefPtr<eventdb::TableReader> table,
      BackfillFnType backfill_fn,
      const String& statefile,
      bool dry_run,
      const Vector<URI>& uris,
      http::HTTPConnectionPool* http);

  void start();
  bool process(size_t batch_size);
  void shutdown();

protected:
  size_t runWorker();
  size_t runUpload();

  RefPtr<eventdb::TableReader> table_;
  RefPtr<eventdb::LogTableTail> tail_;
  BackfillFnType backfill_fn_;
  bool dry_run_;
  String statefile_;
  Vector<URI> target_uris_;
  http::HTTPConnectionPool* http_;
  std::atomic<bool> shutdown_;
  thread::Queue<std::shared_ptr<msg::MessageObject>> inputq_;
  thread::Queue<Buffer> uploadq_;
  size_t num_records_;
  Vector<std::thread> threads_;
  Random rnd_;
};
} // namespace cm

#endif
