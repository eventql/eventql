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
#pragma once
#include "eventql/util/protobuf/MessageSchema.h"
#include "eventql/util/io/inputstream.h"
#include "eventql/db/TSDBService.h"
#include "eventql/auth/internal_auth.h"
#include "eventql/config/namespace_config.h"
#include "eventql/config/config_directory.h"
#include "eventql/db/TableConfig.pb.h"
#include "eventql/LogfileScanResult.h"
#include "eventql/AnalyticsSession.pb.h"
#include "eventql/LogfileScanParams.pb.h"
#include "eventql/sql/runtime/runtime.h"

#include "eventql/eventql.h"

namespace eventql {

class LogfileService {
public:

  LogfileService(
      ConfigDirectory* cdir,
      InternalAuth* auth,
      eventql::TSDBService* tsdb,
      eventql::PartitionMap* pmap,
      eventql::ReplicationScheme* repl,
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
      const NamespaceConfig& cfg);

protected:
  ConfigDirectory* cdir_;
  InternalAuth* auth_;
  eventql::TSDBService* tsdb_;
  eventql::PartitionMap* pmap_;
  eventql::ReplicationScheme* repl_;
  csql::Runtime* sql_;
};

} // namespace eventql
