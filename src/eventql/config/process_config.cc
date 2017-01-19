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
#include <eventql/config/process_config.h>
#include <eventql/util/io/fileutil.h>
#include <eventql/util/human.h>
#include <inih/ini.h>

namespace eventql {

struct IniParserState {
  IniParserState(
      ProcessConfigBuilder* _cfg_builder) :
      cfg_builder(_cfg_builder),
      status(Status::success()) {}
  ProcessConfigBuilder* cfg_builder;
  Status status;
};

ProcessConfig::ProcessConfig(
      HashMap<String, String> properties) :
      properties_(properties) {}

const char* ProcessConfig::getCString(const String& key) const {
  auto p = properties_.find(key);
  if (p != properties_.end()) {
    return p->second.c_str();
  }

  return nullptr;
}

Option<String> ProcessConfig::getString(const String& key) const {
  auto p = properties_.find(key);
  if (p != properties_.end()) {
    return Some(p->second);
  }

  return None<String>();
}

Option<int64_t> ProcessConfig::getInt(const String& key) const {
  auto p = properties_.find(key);
  if (p != properties_.end() && StringUtil::isNumber(p->second)) {
    try {
      auto value = std::stoll(p->second);
      return Some<int64_t>(value);

    } catch (std::exception e) {
      /* fallthrough */
    }
  }

  return None<int64_t>();
}

int64_t ProcessConfig::getInt(const String& key, int64_t or_else) const {
  auto p = properties_.find(key);
  if (p != properties_.end() && StringUtil::isNumber(p->second)) {
    try {
      return std::stoll(p->second);
    } catch (std::exception e) {
      /* fallthrough */
    }
  }

  return or_else;
}

double ProcessConfig::getDouble(const String& key, double or_else) const {
  auto p = properties_.find(key);
  if (p != properties_.end()) {
    try {
      return std::stod(p->second);
    } catch (std::exception e) {
      /* fallthrough */
    }
  }

  return or_else;
}

bool ProcessConfig::getBool(const String& key) const {
  auto p = properties_.find(key);
  if (p != properties_.end()) {
    auto v =  Human::parseBoolean(p->second);
    return v.isEmpty() ? false : v.get();
  }

  return false;
}

bool ProcessConfig::hasProperty(const String& key) const {
  return properties_.find(key) != properties_.end();
}

static int ini_parse_handler(
    void* user,
    const char* section,
    const char* name,
    const char* value) {
  auto parser_state = (IniParserState*) user;
  parser_state->cfg_builder->setProperty(section, name, value);
  return 1;
}

Status ProcessConfigBuilder::loadFile(const String& file) {
  IniParserState parser_state(this);
  if (ini_parse(file.c_str(), &ini_parse_handler, &parser_state) < 0) {
    parser_state.status = Status(eParseError, "invalid config file");
  }

  return parser_state.status;
}

Status ProcessConfigBuilder::loadDefaultConfigFile(const String& process) {
  if (process == "evqld") {
    /* load /etc/evqld.conf */
    auto config_file_path = "/etc/evqld.conf";
    if (FileUtil::exists(config_file_path)) {
      return loadFile(config_file_path);
    } else {
      return Status::success();
    }

  } else {
    /* load /etc/evql.conf */
    auto config_file_path = "/etc/evql.conf";
    if (FileUtil::exists(config_file_path)) {
      auto rc = loadFile(config_file_path);
      if (!rc.isSuccess()) {
        return rc;
      }
    }

    char* homedir = getenv("HOME");
    if (!homedir) {
      return Status::success();
    }

    {
      String confg_file_path = FileUtil::joinPaths(homedir, ".evql.conf");
      if (!FileUtil::exists(confg_file_path)) {
        return Status::success();
      }

      return loadFile(confg_file_path);
    }
  }
}

void ProcessConfigBuilder::setClientDefaults() {
  setProperty("client.timeout", "5000000");
}

void ProcessConfigBuilder::setProperty(const String& key, const String& value) {
  properties_[key] = value;
}

void ProcessConfigBuilder::setProperty(
    const String& section,
    const String& key,
    const String& value) {
  setProperty(StringUtil::format("$0.$1", section, key), value);
}

RefPtr<ProcessConfig> ProcessConfigBuilder::getConfig() {
  return mkRef(new ProcessConfig(properties_));
}

} // namespace eventql

