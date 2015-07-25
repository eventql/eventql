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
#include "stx/io/filerepository.h"
#include "stx/io/fileutil.h"
#include "stx/application.h"
#include "stx/logging.h"
#include "stx/random.h"
#include "stx/uri.h"
#include "stx/thread/eventloop.h"
#include "stx/thread/threadpool.h"
#include "stx/thread/queue.h"
#include "stx/wallclock.h"
#include "stx/cli/flagparser.h"
#include "fnord-logtable/TableRepository.h"
#include "fnord-logtable/LogTableTail.h"
#include "stx/protobuf/MessageEncoder.h"
#include "stx/http/httpconnectionpool.h"

using namespace stx;

namespace cm {

class LogJoinBackfill {
public:
  typedef Function<void (msg::MessageObject* record)> BackfillFnType;

  LogJoinBackfill(
      RefPtr<logtable::TableReader> table,
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

  RefPtr<logtable::TableReader> table_;
  RefPtr<logtable::LogTableTail> tail_;
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
  std::atomic<uint64_t> rr_;
};
} // namespace cm

#endif
