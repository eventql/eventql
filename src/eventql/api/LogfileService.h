/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include "stx/protobuf/MessageSchema.h"
#include "stx/io/inputstream.h"
#include "eventql/core/TSDBService.h"
#include "eventql/AnalyticsAuth.h"
#include "eventql/CustomerConfig.h"
#include "eventql/ConfigDirectory.h"
#include "eventql/TableDefinition.h"
#include "eventql/LogfileScanResult.h"
#include "eventql/AnalyticsSession.pb.h"
#include "eventql/LogfileScanParams.pb.h"
#include "csql/runtime/runtime.h"

using namespace stx;

namespace zbase {

class LogfileService {
public:

  LogfileService(
      ConfigDirectory* cdir,
      AnalyticsAuth* auth,
      zbase::TSDBService* tsdb,
      zbase::PartitionMap* pmap,
      zbase::ReplicationScheme* repl,
      csql::Runtime* sql);

  /**
   * Scan a logfile, returns the "limit" newest rows that match the condition
   * and are older than end_time
   */
  void scanLogfile(
      const AnalyticsSession& session,
      const String& logfile_name,
      const LogfileScanParams& params,
      LogfileScanResult* result,
      Function<void (bool done)> on_progress);

  /**
   * Scan a single logfile partition. This method must be executed on a host
   * that actually stores this partition
   */
  void scanLocalLogfilePartition(
      const AnalyticsSession& session,
      const String& table_name,
      const SHA1Hash& partition,
      const LogfileScanParams& params,
      LogfileScanResult* result);

  /**
   * Scan a single remote logfile partition, try all hosts in order from first
   * to last
   */
  void scanRemoteLogfilePartition(
      const AnalyticsSession& session,
      const String& table_name,
      const SHA1Hash& partition,
      const LogfileScanParams& params,
      const Vector<InetAddr>& hosts,
      LogfileScanResult* result);

  /**
   * Scan a single remote logfile partition on a specific host
   */
  bool scanRemoteLogfilePartition(
      const AnalyticsSession& session,
      const String& table_name,
      const SHA1Hash& partition,
      const LogfileScanParams& params,
      const InetAddr& hosts,
      LogfileScanResult* result);

  void insertLoglines(
      const String& customer,
      const String& logfile_name,
      const Vector<Pair<String, String>>& source_fields,
      InputStream* is);

  void insertLoglines(
      const String& customer,
      const LogfileDefinition& logfile,
      const Vector<Pair<String, String>>& source_fields,
      InputStream* is);

  Option<LogfileDefinition> findLogfileDefinition(
      const String& customer,
      const String& logfile_name);

  void setLogfileRegex(
      const String& customer,
      const String& logfile_name,
      const String& regex);

  RefPtr<msg::MessageSchema> getSchema(
      const LogfileDefinition& cfg);

  Vector<TableDefinition> getTableDefinitions(
      const CustomerConfig& cfg);

protected:
  ConfigDirectory* cdir_;
  AnalyticsAuth* auth_;
  zbase::TSDBService* tsdb_;
  zbase::PartitionMap* pmap_;
  zbase::ReplicationScheme* repl_;
  csql::Runtime* sql_;
};

} // namespace zbase
