/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include <eventql/eventql.h>
#include <eventql/util/exception.h>
#include <eventql/util/stdtypes.h>
#include <eventql/util/cli/CLICommand.h>
#include <eventql/util/status.h>
#include <eventql/cli/cli_config.h>
#include "eventql/server/sql/codec/binary_codec.h"
#include "eventql/sql/result_list.h"
#include "eventql/util/io/TerminalOutputStream.h"
#include "eventql/util/return_code.h"

namespace eventql {
namespace cli {

struct ConsoleOptions {
  String server_host;
  int server_port;
  String database;
  String auth_token;
  String user;
  String password;
  bool batch_mode;
};

class Console {
public:

  Console(const CLIConfig cli_cfg);
  ~Console();

  /**
   * Connect to the server
   */
  ReturnCode connect();

  /**
   * Close the server connection
   */
  void close();

  /**
   * Start an interactive shell. This method will never return
   */
  void startInteractiveShell();

  /**
   * Execute an SQL query
   */
  Status runQuery(const String& query);

  /**
   * Execute a JS job
   */
  Status runJS(const String& query);

protected:

  Status sendRequest(const String& query, csql::BinaryResultParser* res_parser);

  CLIConfig cfg_;
  evql_client_t* client_;
};

} // namespace cli
} // namespace eventql

