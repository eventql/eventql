/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#ifndef _FNORD_TSDB_TSDBCLIENT_H
#define _FNORD_TSDB_TSDBCLIENT_H
#include <eventql/util/stdtypes.h>
#include <eventql/util/random.h>
#include <eventql/util/option.h>
#include <eventql/util/SHA1.h>
#include <eventql/util/http/httpconnectionpool.h>
#include <eventql/db/PartitionInfo.pb.h>
#include <eventql/db/RecordEnvelope.pb.h>

#include "eventql/eventql.h"

namespace eventql {

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
