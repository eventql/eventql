/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include <eventql/cli/cli_config.h>
#include <eventql/util/io/fileutil.h>

namespace eventql {
namespace cli {

const String& CLIConfig::kDefaultHost = "localhost";
const int CLIConfig::kDefaultPort = 9175;
const uint64_t CLIConfig::kDefaultHistoryMaxSize = 100;

CLIConfig::CLIConfig(RefPtr<ProcessConfig> cfg) : cfg_(cfg) {
  auto user = getenv("USER");
  if (user) {
    default_user_ = user;
  }

  auto home = getenv("HOME");
  if (home) {
    default_history_path_ = FileUtil::joinPaths(home, ".evql_history");
  }
}

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

Option<String> CLIConfig::getUser() const {
  auto user = cfg_->getString("evql", "user");
  if (!user.isEmpty()) {
    return Some(user.get());
  } else if (!default_user_.empty()) {
    return Some(default_user_);
  } else {
    return None<String>();
  }
}

bool CLIConfig::getBatchMode() const {
  return cfg_->getBool("evql", "batch");
}

bool CLIConfig::getQuietMode() const {
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

Option<String> CLIConfig::getHistoryPath() const {
  auto path = cfg_->getString("evql", "history_file");
  if (!path.isEmpty()) {
    return Some(path.get());
  } else if (!default_history_path_.empty()) {
    return Some(default_history_path_);
  } else {
    return None<String>();
  }
}

uint64_t CLIConfig::getHistoryMaxSize() const {
  auto max_size = cfg_->getInt("evql", "history_maxlen");
  if (max_size.isEmpty()) {
    return kDefaultHistoryMaxSize;
  } else {
    return max_size.get();
  }
}

} // namespace cli
} // namespace eventql



