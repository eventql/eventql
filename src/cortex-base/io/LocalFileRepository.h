// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <cortex-base/Api.h>
#include <cortex-base/io/FileRepository.h>
#include <functional>
#include <string>

namespace cortex {

class MimeTypes;
class LocalFile;

class CORTEX_API LocalFileRepository : public FileRepository {
 public:
  /**
   * Initializes local file repository.
   *
   * @param mimetypes mimetypes database to use for creating entity tags.
   * @param basedir base directory to start all lookups from (like "/").
   * @param etagMtime whether or not to include Last-Modified timestamp in etag.
   * @param etagSize whether or not to include file size in etag.
   * @param etagInode whether or not to include file's system inode in etag.
   */
  LocalFileRepository(
      MimeTypes& mimetypes,
      const std::string& basedir,
      bool etagMtime, bool etagSize, bool etagInode);

  const std::string baseDirectory() const { return basedir_; }

  std::shared_ptr<File> getFile(
      const std::string& requestPath,
      const std::string& docroot = "/") override;

  void listFiles(std::function<bool(const std::string&)> callback) override;
  void deleteAllFiles() override;
  int createTempFile(std::string* filename = nullptr) override;

  /**
   * Configures ETag generation.
   *
   * @param mtime whether or not to include Last-Modified timestamp
   * @param size whether or not to include file size
   * @param inode whether or not to include file's system inode
   */
  void configureETag(bool mtime, bool size, bool inode);

  bool etagConsiderMTime() const noexcept { return etagConsiderMTime_; }
  bool etagConsiderSize() const noexcept { return etagConsiderSize_; }
  bool etagConsiderINode() const noexcept { return etagConsiderINode_; }

 private:
  friend class LocalFile;

  MimeTypes& mimetypes_;
  std::string basedir_;
  bool etagConsiderMTime_;
  bool etagConsiderSize_;
  bool etagConsiderINode_;
};

}  // namespace cortex

