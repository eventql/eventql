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
#include <inih/ini.h>

namespace eventql {
namespace cli {

CliConfig::CliConfig() {}

CliConfig::CliConfig(const String& config_file) {
  loadConfigFile(config_file);
}

void CliConfig::loadConfigFile(const String& config_file) {

}

Option<String> CliConfig::getHost() const {
  return server_host_;
}

Option<int> CliConfig::getPort() const {
  return server_port_;
}

Option<String> CliConfig::getAuthToken() const {
  return server_auth_token_;
}

Option<String> CliConfig::getFile() const {
  return file_;
}

Option<String> CliConfig::getExec() const {
  return exec_;
}

Option<bool> CliConfig::getBatchMode() const {
  return batch_mode_;
}

} // namespace cli
} // namespace eventql



