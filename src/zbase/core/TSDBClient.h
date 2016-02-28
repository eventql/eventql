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
#include <stx/stdtypes.h>
#include <stx/random.h>
#include <stx/option.h>
#include <stx/SHA1.h>
#include <stx/http/httpconnectionpool.h>
#include <zbase/core/PartitionInfo.pb.h>
#include <zbase/core/RecordEnvelope.pb.h>

using namespace stx;

namespace zbase {

class TSDBClient {
public:
  size_t kMaxInsertBachSize = 1024;

  TSDBClient(
      const String& uri,
      http::HTTPConnectionPool* http);

  void insertRecord(const RecordEnvelope& record);
  void insertRecords(const RecordEnvelopeList& records);

  void insertRecord(
      const String& tsdb_namespace,
      const String& table_name,
      const SHA1Hash& partition_key,
      const SHA1Hash& record_id,
      const Buffer& record_data);

  //Vector<String> listPartitions(
  //    const String& table_name,
  //    const UnixTime& from,
  //    const UnixTime& until);

  void fetchPartition(
      const String& tsdb_namespace,
      const String& table_name,
      const SHA1Hash& partition_key,
      Function<void (const Buffer& record)> fn);

  void fetchPartitionWithSampling(
      const String& tsdb_namespace,
      const String& table_name,
      const SHA1Hash& partition_key,
      size_t sample_modulo,
      size_t sample_index,
      Function<void (const Buffer& record)> fn);

  Option<PartitionInfo> partitionInfo(
      const String& tsdb_namespace,
      const String& table_name,
      const SHA1Hash& partition_key);

  //Buffer fetchDerivedDataset(
  //    const String& table_name,
  //    const String& partition,
  //    const String& derived_dataset_name);

  uint64_t mkMessageID();

protected:

  void insertRecordsToHost(
      const String& host,
      const RecordEnvelopeList& records);

  String uri_;
  http::HTTPConnectionPool* http_;
  Random rnd_;
};

} // namespace tdsb

#endif
