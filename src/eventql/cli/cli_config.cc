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

namespace eventql {
namespace cli {

const String& CLIConfig::kDefaultHost = "localhost";
const int CLIConfig::kDefaultPort = 9175;
const String& CLIConfig::kDefaultUser = getenv("USER");

CLIConfig::CLIConfig(RefPtr<ProcessConfig> cfg) : cfg_(cfg) {}

String CLIConfig::getHost() const {
  auto host = cfg_->getString("evql", "host");
  if (host.isEmpty()) {
    return kDefaultHost;
  } else {
    return host.get();
  }
}

int CLIConfig::getPort() const {
  auto port = cfg_->getInt("evql", "port");
  if (port.isEmpty()) {
    return kDefaultPort;
  } else {
    return port.get();
  }
}

String CLIConfig::getUser() const {
  auto user = cfg_->getString("evql", "user");
  if (user.isEmpty()) {
    return kDefaultUser;
  } else {
    return user.get();
  }
}

bool CLIConfig::getBatchMode() const {
  return cfg_->getBool("evql", "batch");
}

bool CLIConfig::getQuiet() const {
  return cfg_->getBool("evql", "quiet");
}

Option<String> CLIConfig::getDatabase() const {
  return cfg_->getString("evql", "database");
}

Option<String> CLIConfig::getPassword() const {
  return cfg_->getString("evql", "password");
}

Option<String> CLIConfig::getAuthToken() const {
  return cfg_->getString("evql", "auth_token");
}

Option<String> CLIConfig::getFile() const {
  return cfg_->getString("evql", "file");
}

Option<CLIConfig::kLanguage> CLIConfig::getLanguage() {
  auto l = cfg_->getString("evql", "lang");
  if (!l.isEmpty()) {
    auto language = l.get();
    StringUtil::toUpper(&language);

    if (language == "SQL") {
      return Some(CLIConfig::kLanguage::SQL);
    } else if (language == "JS" || language == "JAVASCRIPT") {
      return Some(CLIConfig::kLanguage::JAVASCRIPT);
    }

  } else {

    auto file = cfg_->getString("evql", "file");
    if (!file.isEmpty()) {
      if (StringUtil::endsWith(file.get(), ".sql")) {
        return Some(CLIConfig::kLanguage::SQL);
      } else if (StringUtil::endsWith(file.get(), ".js")) {
        return Some(CLIConfig::kLanguage::JAVASCRIPT);
      }
    }
  }

  return None<CLIConfig::kLanguage>();
}

Option<String> CLIConfig::getExec() const {
  return cfg_->getString("evql", "exec");
}


} // namespace cli
} // namespace eventql



