/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Laura Schlimmer <laura@eventql.io>
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
#include "eventql/util/cli/flagparser.h"
#include "eventql/cli/commands/table_export.h"

namespace eventql {
namespace cli {

const std::string TableExport::kName_ = "table-export";
const std::string TableExport::kDescription_ = "Export a table to a csv file";

TableExport::TableExport(
    RefPtr<ProcessConfig> process_cfg) :
    process_cfg_(process_cfg),
    client_(nullptr) {}

Status TableExport::execute(
    const std::vector<std::string>& argv,
    FileInputStream* stdin_is,
    OutputStream* stdout_os,
    OutputStream* stderr_os) {
  ::cli::FlagParser flags;

  flags.defineFlag(
      "database",
      ::cli::FlagParser::T_STRING,
      true,
      "d",
      NULL,
      "database",
      "<string>");

  flags.defineFlag(
      "table",
      ::cli::FlagParser::T_STRING,
      true,
      "t",
      NULL,
      "table",
      "<string>");

  flags.defineFlag(
      "file",
      ::cli::FlagParser::T_STRING,
      true,
      "f",
      NULL,
      "file",
      "<string>");

  flags.defineFlag(
      "host",
      ::cli::FlagParser::T_STRING,
      true,
      "h",
      NULL,
      "host",
      "<string>");

  flags.defineFlag(
      "port",
      ::cli::FlagParser::T_INTEGER,
      true,
      "p",
      NULL,
      "port",
      "<integer>");

  try {
    flags.parseArgv(argv);

  } catch (const Exception& e) {

    stderr_os->write(StringUtil::format(
        "$0: $1\n",
       e.getTypeName(),
        e.getMessage()));
    return Status(e);
  }

  auto rc = connect(
      flags.getString("database"),
      flags.getString("host"),
      flags.getInt("port"));

  if (!rc.isSuccess()) {
    close();
    return rc;
  }

  auto query = StringUtil::format(
      "SELECT * FROM $0;",
      flags.getString("table"));


  stderr_os->write("exporting table");

  return Status::success();
}

Status TableExport::connect(
    const std::string& database,
    const std::string& host,
    const uint32_t port) {
  if (!client_) {
    client_ = evql_client_init();
    if (!client_) {
      return Status(eRuntimeError, "can't initialize eventql client");
    }
  }

  //{
  //  uint64_t timeout = cfg_.getTimeout();
  //  auto rc = evql_client_setopt(
  //      client_,
  //      EVQL_CLIENT_OPT_TIMEOUT,
  //      (const char*) &timeout,
  //      sizeof(timeout),
  //      0);

  //  if (rc != 0) {
  //    return Status(eRuntimeError, "can't initialize eventql client");
  //  }
  //}

  if (process_cfg_->hasProperty("client.user")) {
    std::string akey = "user";
    std::string aval = process_cfg_->getString("client.user").get();
    evql_client_setauth(
        client_,
        akey.data(),
        akey.size(),
        aval.data(),
        aval.size(),
        0);
  }

  if (process_cfg_->hasProperty("client.password")) {
    std::string akey = "password";
    std::string aval = process_cfg_->getString("client.password").get();
    evql_client_setauth(
        client_,
        akey.data(),
        akey.size(),
        aval.data(),
        aval.size(),
        0);
  }

  if (process_cfg_->hasProperty("client.auth_token")) {
    std::string akey = "auth_token";
    std::string aval = process_cfg_->getString("client.auth_token").get();
    evql_client_setauth(
        client_,
        akey.data(),
        akey.size(),
        aval.data(),
        aval.size(),
        0);
  }

  auto rc = evql_client_connect(
      client_,
      host.c_str(),
      port,
      database.c_str(),
      0);

  if (rc < 0) {
    return Status(eIOError, evql_client_geterror(client_));
  }

  return Status::success();
}

void TableExport::close() {
  if (client_) {
    evql_client_close(client_);
  }
}

const std::string& TableExport::getName() const {
  return kName_;
}

const std::string& TableExport::getDescription() const {
  return kDescription_;
}

void TableExport::printHelp(OutputStream* stdout_os) const {
  stdout_os->write(StringUtil::format(
      "\nevqlctl-$0 - $1\n\n", kName_, kDescription_));

  stdout_os->write(
      "Usage: evqlctl table-export [OPTIONS]\n"
      "  -d, --database <db>          Select a database.\n"
      "  -t, --table <tbl>            Select a destination table.\n"
      "  -f, --file <file>            Set the path of the output file.\n"
      "  -h, --host <hostname>        Set the EventQL hostname.\n"
      "  -p, --port <port>            Set the EventQL port.\n");
}


} //namespace cli
} //namespace eventql

