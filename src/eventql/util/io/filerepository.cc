/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include <eventql/util/random.h>
#include <eventql/util/io/filerepository.h>
#include <eventql/util/io/fileutil.h>

FileRepository::FileRepository(
    const std::string& basedir) :
    basedir_(basedir) {}

FileRepository::FileRef FileRepository::createFile() const {
  FileRef fileref;
  fileref.logical_filename = rnd_.alphanumericString(32);
  fileref.absolute_path = FileUtil::joinPaths(
      basedir_,
      fileref.logical_filename);

  return fileref;
}

void FileRepository::listFiles(
    std::function<bool(const std::string&)> callback) const {
  FileUtil::ls(basedir_, [&] (const std::string& filename) -> bool {
    auto absolute_path = FileUtil::joinPaths(basedir_, filename);
    return callback(absolute_path);
  });
}

void FileRepository::deleteAllFiles() {
  listFiles([] (const std::string& filename) -> bool {
    FileUtil::rm(filename);
    return true;
  });
}

