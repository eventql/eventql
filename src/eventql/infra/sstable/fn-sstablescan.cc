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
#include <stdlib.h>
#include <unistd.h>
#include "eventql/util/application.h"
#include "eventql/util/cli/flagparser.h"
#include "eventql/util/logging.h"
#include "eventql/util/inspect.h"
#include "eventql/infra/sstable/sstablereader.h"
#include "eventql/infra/sstable/SSTableScan.h"

#include "eventql/eventql.h"

int main(int argc, const char** argv) {
  util::Application::init();
  util::Application::logToStderr();

  cli::FlagParser flags;

  flags.defineFlag(
      "file",
      cli::FlagParser::T_STRING,
      true,
      "f",
      NULL,
      "input sstable file",
      "<file>");

  flags.defineFlag(
      "limit",
      cli::FlagParser::T_INTEGER,
      false,
      NULL,
      NULL,
      "limit",
      "<num>");

  flags.defineFlag(
      "offset",
      cli::FlagParser::T_INTEGER,
      false,
      NULL,
      NULL,
      "offset",
      "<num>");

  flags.defineFlag(
      "order_by",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "order by",
      "<column>");

  flags.defineFlag(
      "order_fn",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      "STRASC",
      "one of: STRASC, STRDSC, NUMASC, NUMDSC",
      "<fn>");

  flags.defineFlag(
      "loglevel",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      "INFO",
      "loglevel",
      "<level>");

  flags.parseArgv(argc, argv);

  Logger::get()->setMinimumLogLevel(
      strToLogLevel(flags.getString("loglevel")));

  /* open input sstable */
  auto input_file = flags.getString("file");
  sstable::SSTableReader reader(File::openFile(input_file, File::O_READ));
  if (reader.bodySize() == 0) {
    logWarning("fnord.sstablescan", "sstable is unfinished");
  }

  sstable::SSTableColumnSchema schema;
  schema.loadIndex(&reader);

  /* set up scan */
  sstable::SSTableScan scan(&schema);
  if (flags.isSet("limit")) {
    scan.setLimit(flags.getInt("limit"));
  }

  if (flags.isSet("offset")) {
    scan.setOffset(flags.getInt("offset"));
  }

  if (flags.isSet("order_by")) {
    scan.setOrderBy(flags.getString("order_by"), flags.getString("order_fn"));
  }

  /* execute scan */
  auto headers = scan.columnNames();
  iputs("$0", StringUtil::join(headers, ";"));

  auto cursor = reader.getCursor();
  scan.execute(cursor.get(), [] (const Vector<String> row) {
    iputs("$0", StringUtil::join(row, ";"));
  });

  return 0;
}

