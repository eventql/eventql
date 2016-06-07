/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *   - Laura Schlimmer <laura@zscale.io>
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
#include <eventql/cli/cli_config.h>
#include <eventql/util/inspect.h>
#include <eventql/util/io/fileutil.h>
#include <inih/ini.h>

namespace eventql {
namespace cli {

struct IniParserState {
  IniParserState(CLIConfig* _cfg) : cfg(_cfg), status(Status::success()) {}
  CLIConfig* cfg;
  Status status;
};

static int ini_parse_handler(
    void* user,
    const char* section,
    const char* name,
    const char* value) {
  auto parser_state = (IniParserState*) user;
  auto status = parser_state->cfg->setConfigOption(section, name, value);
  if (status.isSuccess()) {
    return 1;
  } else {
    parser_state->status = status;
    return 0;
  }
}

CLIConfig::CLIConfig() :
    server_host_("localhost"),
    server_port_(9175) {
  user_ = getenv("USER");
  if (user_.empty()) {
    user_ = "nobody";
  }
}

Status CLIConfig::loadDefaultConfigFile() {
  char* homedir = getenv("HOME");
  if (!homedir) {
    return Status::success();
  }

  String confg_file_path = FileUtil::joinPaths(homedir, ".evqlrc");
  if (!FileUtil::exists(confg_file_path)) {
    return Status::success();
  }

  return loadConfigFile(confg_file_path);
}

Status CLIConfig::loadConfigFile(const String& config_file) {
  IniParserState parser_state(this);
  if (ini_parse(config_file.c_str(), &ini_parse_handler, &parser_state) < 0) {
    parser_state.status = Status(eParseError, "invalid config file");
  }

  return parser_state.status;
}

Status CLIConfig::setConfigOption(
    const String& section,
    const String& key,
    const String& value) {
  if (section != "evql") {
    return Status(
        eParseError,
        StringUtil::format("section '$0' is not valid", section));
  }

  if (value.size() == 0) {
    return Status(
        eParseError,
        StringUtil::format("'$0' value '$1' is not valid", key, value));
  }

  if (key == "host") {
    return setHost(value);

  } else if (key == "port") {
    return setPort(value);

  } else if (key == "auth_token") {
    return setAuthToken(value);

  } else {
    return Status(
        eParseError,
        StringUtil::format("config file option '$0' is not valid", key));
  }
}

Status CLIConfig::setDatabase(const String& database) {
  database_ = database;
  return Status::success();
}

String CLIConfig::getDatabase() const {
  return database_;
}

Status CLIConfig::setUser(const String& user) {
  user_ = user;
  return Status::success();
}

String CLIConfig::getUser() const {
  return user_;
}

Status CLIConfig::setPassword(const String& password) {
  password_ = password;
  return Status::success();
}

String CLIConfig::getPassword() const {
  return password_;
}

Status CLIConfig::setHost(const String& host /* = "localhost" */) {
  server_host_ = host; //FIXME check host format
  return Status::success();
}

Status CLIConfig::setPort(const int port /* = 80 */) {
  if (port < 0 || port > 65535) {
    return Status(
        eFlagError,
        StringUtil::format("'port' value '$0' is not valid", port));
  }

  server_port_ = port;
  return Status::success();
}

Status CLIConfig::setPort(const String& port) {
  if (!StringUtil::isNumber(port)) {
    return Status(
        eFlagError,
        StringUtil::format("'port' value '$0' is not a valid integer", port));
  }

  return setPort(stoi(port));
}

Status CLIConfig::setAuthToken(const String& auth_token) {
  server_auth_token_ = auth_token;
  return Status::success();
}

void CLIConfig::enableBatchMode() {
  batch_mode_ = true;
}

String CLIConfig::getHost() const {
  return server_host_;
}

int CLIConfig::getPort() const {
  return server_port_;
}

Option<String> CLIConfig::getAuthToken() const {
  if (server_auth_token_.size() > 0) {
    return Some<String>(server_auth_token_);
  } else {
    return None<String>();
  }
}

Status CLIConfig::setFile(const String& file) {
  file_ = file;
  return Status::success();
}

Option<String> CLIConfig::getFile() const {
  if (file_.empty()) {
    return None<String>();
  } else {
    return Some(file_);
  }
}

Status CLIConfig::setLanguage(String language) {
  StringUtil::toUpper(&language);

  if (language == "SQL") {
    language_ = Some(CLIConfig::kLanguage::SQL);
    return Status::success();

  } else if (language == "JS" || language == "JAVASCRIPT") {
    language_ = Some(CLIConfig::kLanguage::JAVASCRIPT);
    return Status::success();

  } else {
    return Status(
        eFlagError,
        StringUtil::format("invalid language '$0'", language));
  }
}

Option<CLIConfig::kLanguage> CLIConfig::getLanguage() {
  if (!language_.isEmpty()) {
    return language_;
  }

  if (!file_.empty()) {
    if (StringUtil::endsWith(file_, ".sql")) {
      return Some(CLIConfig::kLanguage::SQL);
    }
    if (StringUtil::endsWith(file_, ".js")) {
      return Some(CLIConfig::kLanguage::JAVASCRIPT);
    }
  }

  return None<CLIConfig::kLanguage>();
}
Option<String> CLIConfig::getExec() const {
  return exec_;
}

bool CLIConfig::batchModeEnabled() const {
  return batch_mode_;
}

} // namespace cli
} // namespace eventql



