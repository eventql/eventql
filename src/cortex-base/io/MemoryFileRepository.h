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
#include <cortex-base/io/MemoryFile.h>
#include <unordered_map>
#include <string>

namespace cortex {

class MimeTypes;

/**
 * In-memory file store.
 *
 * @see LocalFileRepository
 * @see FileHandler
 * @see MemoryFile
 */
class CORTEX_API MemoryFileRepository : public FileRepository {
 public:
  explicit MemoryFileRepository(MimeTypes& mimetypes);

  std::shared_ptr<File> getFile(
      const std::string& requestPath,
      const std::string& docroot = "/") override;

  void listFiles(std::function<bool(const std::string&)> callback) override;
  void deleteAllFiles() override;
  int createTempFile(std::string* filename = nullptr) override;

  void insert(const std::string& path, const BufferRef& data, DateTime mtime);

  void insert(const std::string& path, const BufferRef& data);

 private:
  MimeTypes& mimetypes_;
  std::unordered_map<std::string, std::shared_ptr<MemoryFile>> files_;
  std::shared_ptr<MemoryFile> notFound_;
};

}  // namespace cortex

