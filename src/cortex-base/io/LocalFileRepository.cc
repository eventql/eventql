// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/io/LocalFileRepository.h>
#include <cortex-base/io/LocalFile.h>
#include <cortex-base/io/FileUtil.h>
#include <cortex-base/MimeTypes.h>

namespace cortex {

LocalFileRepository::LocalFileRepository(MimeTypes& mt,
                                                 const std::string& basedir,
                                                 bool etagMtime,
                                                 bool etagSize,
                                                 bool etagInode)
    : mimetypes_(mt),
      basedir_(FileUtil::realpath(basedir)),
      etagConsiderMTime_(etagMtime),
      etagConsiderSize_(etagSize),
      etagConsiderINode_(etagInode) {
}

std::shared_ptr<File> LocalFileRepository::getFile(
    const std::string& requestPath,
    const std::string& docroot) {

  std::string path = FileUtil::joinPaths(FileUtil::joinPaths(basedir_, docroot), requestPath);

  return std::shared_ptr<File>(new LocalFile(
        *this, path, mimetypes_.getMimeType(requestPath)));
}

void LocalFileRepository::listFiles(
    std::function<bool(const std::string&)> callback) {
  FileUtil::ls(basedir_,
               [&](const std::string& filename) -> bool {
                  return callback(FileUtil::joinPaths(basedir_, filename));
               });
}

void LocalFileRepository::deleteAllFiles() {
  FileUtil::ls(basedir_,
               [&](const std::string& filename) -> bool {
                  FileUtil::rm(FileUtil::joinPaths(basedir_, filename));
                  return true;
               });
}

int LocalFileRepository::createTempFile(std::string* filename) {
  if (basedir_.empty() || basedir_ == "/")
    return FileUtil::createTempFileAt(FileUtil::tempDirectory(), filename);
  else
    return FileUtil::createTempFileAt(basedir_, filename);
}

void LocalFileRepository::configureETag(bool mtime, bool size, bool inode) {
  etagConsiderMTime_ = mtime;
  etagConsiderSize_ = size;
  etagConsiderINode_ = inode;
}

}  // namespace cortex
