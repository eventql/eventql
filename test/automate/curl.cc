/**
 * Copyright (c) 2017 DeepCortex GmbH <legal@eventql.io>
 * Authors:
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
#include "eventql/util/io/fileutil.h"
#include "eventql/util/io/inputstream.h"
#include "automate/curl.h"

namespace eventql {
namespace test {

const std::string kResultFileEnding = ".result.txt";
const std::string kOutputFileEnding = ".actual.result.txt";

void executePOST(
    TestContext* ctx,
    const std::string& test_path,
    const std::string& path,
    const std::string& host,
    const std::string& port,
    const std::string& body) {
  auto abs_test_path = FileUtil::joinPaths(ctx->srcdir, test_path);
  auto output_file_path = abs_test_path + kOutputFileEnding;
  auto result_file_path = abs_test_path + kResultFileEnding;
  auto api_path = FileUtil::joinPaths(kAPIPath, path);

  Process::runOrDie(
      "/usr/bin/curl",
      {
        "-X", "POST",
        "-v",
        "--write-out", "\"HTTP-CODE %{http_code}\"",
        "-d", body,
        "-o", output_file_path,
        StringUtil::format("$0:$1$2", host, port, api_path)
      },
      {},
      "curl-post",
      ctx->log_fd);

  if (FileUtil::exists(output_file_path)) {
    auto output_is = FileInputStream::openFile(output_file_path);

    if (FileUtil::exists(result_file_path)) {
      auto result_is = FileInputStream::openFile(result_file_path);
      std::string result_line;
      std::string output_line;
      while (output_is->readLine(&output_line)) {
        if (!result_is->readLine(&result_line)) {
          RAISE(kRuntimeError, "too many rows returned");
        }

        StringUtil::chomp(&result_line);
        StringUtil::chomp(&output_line);

        if (result_line != output_line) {
          RAISE(
              kRuntimeError,
              StringUtil::format("expected $0 to be $1", output_line, result_line));
        }

        result_line.clear();
        output_line.clear();
      }

      if (result_is->readLine(&result_line)) {
        RAISE(kRuntimeError, "not enough rows returned");
      }

    } else {
      auto output_buf = FileUtil::read(output_file_path);
      if (output_buf.size() > 0) {
        RAISE(kRuntimeError, "got result but none was expected");
      }
    }

  } else if (FileUtil::exists(result_file_path)) {
    auto result_buf = FileUtil::read(result_file_path);
    if (result_buf.size() > 0) {
      RAISE(kRuntimeError, "expected_result but none was returned");
    }
  }
}

} // namespace test
} // namespace eventql


