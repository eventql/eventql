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
#pragma once
#include <eventql/eventql.h>
#include <eventql/util/stdtypes.h>
#include <eventql/util/option.h>
#include <eventql/util/status.h>
#include <eventql/config/process_config.h>

namespace eventql {
namespace cli {

class CLIConfig {
public:

  static const String& kDefaultHost;
  static const int kDefaultPort;
  static const uint64_t kDefaultHistoryMaxSize;

  enum class kLanguage { SQL, JAVASCRIPT };

  CLIConfig(RefPtr<ProcessConfig> cfg);

  String getHost() const;

  int getPort() const;

  Option<String> getUser() const;

  bool getBatchMode() const;

  bool getQuietMode() const;

  Option<String> getDatabase() const;

  Option<String> getPassword() const;

  Option<String> getAuthToken() const;

  Option<String> getFile() const;

  Option<kLanguage> getLanguage();

  Option<String> getExec() const;

  Option<String> getHistoryPath() const;

  uint64_t getHistoryMaxSize() const;

protected:
  RefPtr<ProcessConfig> cfg_;
  String default_user_;
  String default_history_path_;
};

} // namespace cli
} // namespace eventql


