/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include "eventql/util/protobuf/MessageSchema.h"
#include "eventql/util/io/inputstream.h"
#include "eventql/core/TSDBService.h"
#include "eventql/AnalyticsAuth.h"
#include "eventql/CustomerConfig.h"
#include "eventql/ConfigDirectory.h"
#include "eventql/TableDefinition.h"
#include "eventql/LogfileScanResult.h"
#include "eventql/AnalyticsSession.pb.h"
#include "eventql/LogfileScanParams.pb.h"
#include "eventql/sql/runtime/runtime.h"

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
