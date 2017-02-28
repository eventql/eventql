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

  std::cerr << "BODY -------- " << body << std::endl;
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
    //auto result_is = FileInputStream::openFile(result_file_path);
    auto output_is = FileInputStream::openFile(output_file_path);

    std::string line;

    while (output_is->readLine(&line)) {
      std::cerr << line << std::endl;
      line.clear();
    }
  }
}

} // namespace test
} // namespace eventql


