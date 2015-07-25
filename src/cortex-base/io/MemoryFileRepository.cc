// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/io/MemoryFileRepository.h>
#include <cortex-base/RuntimeError.h>
#include <cortex-base/MimeTypes.h>

namespace cortex {

MemoryFileRepository::MemoryFileRepository(MimeTypes& mimetypes)
    : mimetypes_(mimetypes),
      files_(),
      notFound_(new MemoryFile()) {
}

std::shared_ptr<File> MemoryFileRepository::getFile(
    const std::string& requestPath,
    const std::string& /*docroot*/) {
  auto i = files_.find(requestPath);
  if (i != files_.end())
    return i->second;

  return notFound_;
}

void MemoryFileRepository::listFiles(
    std::function<bool(const std::string&)> callback) {
  for (auto& file: files_) {
    if (!callback(file.first)) {
      break;
    }
  }
}

void MemoryFileRepository::deleteAllFiles() {
  files_.clear();
}

int MemoryFileRepository::createTempFile(std::string* filename) {
  RAISE_STATUS(NotImplementedError);
}

void MemoryFileRepository::insert(
    const std::string& path, const BufferRef& data, DateTime mtime) {
  files_[path].reset(new MemoryFile(path,
                                    mimetypes_.getMimeType(path),
                                    data,
                                    mtime));
}

void MemoryFileRepository::insert(
    const std::string& path, const BufferRef& data) {
  insert(path, data, DateTime());
}

} // namespace cortex
