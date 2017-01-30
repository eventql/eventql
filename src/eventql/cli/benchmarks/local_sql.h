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
#include "eventql/cli/commands/cli_command.h"
#include "eventql/config/process_config.h"
#include "eventql/sql/runtime/defaultruntime.h"

namespace eventql {
namespace cli {

class LocalSQLBenchmark : public CLICommand {
public:

  static const std::string kName_;
  static const std::string kDescription_;

  LocalSQLBenchmark();

  Status execute(
      const std::vector<std::string>& argv,
      FileInputStream* stdin_is,
      OutputStream* stdout_os,
      OutputStream* stderr_os) override;

  const String& getName() const override;
  const String& getDescription() const override;
  void printHelp(OutputStream* stdout_os) const override;

protected:

  ReturnCode runQuery(csql::Runtime* runtime) const;

  bool verbose_;
  std::string query_;
};

} // namespace cli
} // namespace eventql

