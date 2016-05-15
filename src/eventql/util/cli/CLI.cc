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
#include <eventql/util/cli/CLI.h>

namespace stx {
namespace cli {

RefPtr<CLICommand> CLI::defineCommand(const String& command) {
  RefPtr<CLICommand> cmd(new CLICommand(command));
  commands_.emplace(command, cmd);
  return cmd;
}

void CLI::call(const std::vector<std::string>& argv) {
  if (argv.size() == 0) {
    RAISE(kUsageError, "no command provided");
  }

  auto cmd_name = argv[0];
  auto cmd_argv = argv;
  cmd_argv.erase(cmd_argv.begin());

  auto cmd_iter = commands_.find(cmd_name);
  if (cmd_iter == commands_.end()) {
    RAISEF(kUsageError, "command not found: $0", cmd_name);
  }

  auto& cmd = cmd_iter->second;
  cmd->call(cmd_argv);
}


}
}
