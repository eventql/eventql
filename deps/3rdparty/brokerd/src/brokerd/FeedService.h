/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_FEEDS_H
#define _FNORD_FEEDS_H
#include <mutex>
#include <stdlib.h>
#include <set>
#include <string>
#include <unordered_map>
#include "stx/stdtypes.h"
#include "stx/random.h"
#include "stx/io/filerepository.h"
#include "stx/io/FileLock.h"
#include "brokerd/LocalFeed.h"
#include "brokerd/FeedEntry.h"
#include "brokerd/Message.pb.h"
#include "stx/reflect/reflect.h"

namespace stx {
namespace feeds {

class FeedService {
  friend class LogStream;
public:

  FeedService(
      const String& data_dir,
      const String& stats_path = "/brokerd");

  /**
   * DEPRECATED
   *
   * Append an entry to the stream referenced by `stream` and return the offset
   * at which the entry was written. Will create a new stream if the referenced
   * stream does not exist yet.
   *
   * @param stream the name/key of the stream to append to
   * @param entry the entry to append to the stream
   * @return the offset at which the entry was written
   */
  uint64_t append(String stream, String entry); // FIXPAUL DEPRECATED

  /**
   * Insert a record into the topic referenced by `topic` and return the offset
   * at which the record was written. Will create a new topic if the referenced
   * topic does not exist yet.
   *
   * @param topic the name/key of the topic
   * @param record the record to append to the topic
   * @return the offset at which the record was written
   */
  uint64_t insert(const String& topic, const Buffer& record);

  /**
   * Read one or more entries from the stream at or after the provided start
   * offset. If the start offset references a deleted/expired entry, the next
   * valid entry will be returned. It is always valid to call this method with
   * a start offset of zero to retrieve the first entry or entries from the
   * stream.
   *
   * @param start_offset the start offset to read from
   */
  std::vector<FeedEntry> fetch(
      std::string stream,
      uint64_t offset,
      int batch_size);

  /**
   * Read one or more entries from the stream at or after the provided start
   * offset. If the start offset references a deleted/expired entry, the next
   * valid entry will be returned. It is always valid to call this method with
   * a start offset of zero to retrieve the first entry or entries from the
   * stream.
   *
   * @param start_offset the start offset to read from
   */
  void fetchSome(
      std::string stream,
      uint64_t offset,
      int batch_size,
      Function<void (const Message& msg)> fn);

  String hostID();

protected:
  String stats_path_;

  LogStream* openStream(const std::string& name, bool create);
  void reopenTable(const std::string& file_path);

  stx::FileRepository file_repo_;
  std::unordered_map<std::string, std::unique_ptr<LogStream>> streams_;
  std::mutex streams_mutex_;
  FileLock lock_;
  String hostid_;
  Random rnd_;
};

} // namespace logstream_service
} // namespace stx

template <> template <class T>
void stx::reflect::MetaClass<
    stx::feeds::FeedService>::reflect(T* t) {
  t->method(
      "FeedService.append",
      &stx::feeds::FeedService::append,
      "stream",
      "entry");

  t->method(
      "FeedService.fetch",
      &stx::feeds::FeedService::fetch,
      "stream",
      "offset",
      "batch_size");
}

#endif
