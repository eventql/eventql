/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_TSDB_TSDBCLIENT_H
#define _FNORD_TSDB_TSDBCLIENT_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/random.h>
#include <fnord-base/option.h>
#include <fnord-http/httpconnectionpool.h>
#include <fnord-tsdb/PartitionInfo.pb.h>

namespace fnord {
namespace tsdb {

class TSDBClient {
public:

  TSDBClient(
      const String& uri,
      http::HTTPConnectionPool* http);

  Vector<String> listPartitions(
      const String& stream_key,
      const DateTime& from,
      const DateTime& until);

  void fetchPartition(
      const String& stream_key,
      const String& partition_key,
      Function<void (const Buffer& record)> fn);

  PartitionInfo fetchPartitionInfo(
      const String& stream_key,
      const String& partition_key);

  void fetchPartitionWithSampling(
      const String& stream_key,
      const String& partition,
      size_t sample_modulo,
      size_t sample_index,
      Function<void (const Buffer& record)> fn);

  Buffer fetchDerivedDataset(
      const String& stream_key,
      const String& partition,
      const String& derived_dataset_name);

protected:
  String uri_;
  http::HTTPConnectionPool* http_;
};

} // namespace tdsb
} // namespace fnord

#endif
