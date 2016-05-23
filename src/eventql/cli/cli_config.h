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
#pragma once
#include <eventql/eventql.h>
#include <eventql/util/stdtypes.h>
#include <eventql/util/option.h>
#include <eventql/util/status.h>

namespace eventql {
namespace cli {

class CLIConfig {
public:

  CLIConfig();

  Status loadDefaultConfigFile();
  Status loadConfigFile(const String& file_path);

  Status setHost(const String& host);
  Status setPort(const String& port);
  Status setAuthToken(const String& auth_token);
  Status setBatchMode(const String& batch_mode);

  Option<String> getHost() const;
  Option<int> getPort() const;
  Option<String> getAuthToken() const;
  Option<bool> getBatchMode() const;
  Option<String> getFile() const;
  Option<String> getExec() const;

  Status setConfigOption(
      const String& section,
      const String& key,
      const String& value);

protected:
  String server_host_;
  int server_port_;
  String server_auth_token_;
  bool batch_mode_;
  String file_;
  String exec_;
};

} // namespace cli
} // namespace eventql


