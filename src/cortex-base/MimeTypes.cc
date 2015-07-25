// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/MimeTypes.h>
#include <cortex-base/io/FileUtil.h>
#include <cortex-base/Tokenizer.h>
#include <cortex-base/Buffer.h>

namespace cortex {

MimeTypes::MimeTypes()
    : MimeTypes("/etc/mime.types", "application/octet-stream") {
}

MimeTypes::MimeTypes(const std::string& path,
                     const std::string& defaultMimeType)
    : mimetypes_(),
      defaultMimeType_(defaultMimeType) {

  if (!path.empty()) {
    loadFromLocal(path);
  }
}

/** Loads the mimetype map from given local file at @p path. */
void MimeTypes::loadFromLocal(const std::string& path) {
  Buffer input = FileUtil::read(path);

  mimetypes_.clear();
  auto lines = Tokenizer<BufferRef, Buffer>::tokenize(input, "\n");

  for (auto line : lines) {
    line = line.trim();
    auto columns = Tokenizer<BufferRef, BufferRef>::tokenize(line, " \t");

    auto ci = columns.begin(), ce = columns.end();
    BufferRef mime = ci != ce ? *ci++ : BufferRef();

    if (!mime.empty() && mime[0] != '#') {
      for (; ci != ce; ++ci) {
        mimetypes_[ci->str()] = mime.str();
      }
    }
  }
}

const std::string& MimeTypes::getMimeType(const std::string& path) {
  std::string* result = nullptr;

  // query mimetype
  const size_t ndot = path.find_last_of(".");
  const size_t nslash = path.find_last_of("/");

  if (ndot != std::string::npos && ndot > nslash) {
    std::string ext(path.substr(ndot + 1));

    while (ext.size()) {
      auto i = mimetypes_.find(ext);

      if (i != mimetypes_.end())
        result = &i->second;

      if (ext[ext.size() - 1] != '~')
        break;

      ext.resize(ext.size() - 1);
    }

    if (!result || result->empty()) {
      result = &defaultMimeType_;
    }
  } else {
    result = &defaultMimeType_;
  }

  return *result;
}

}  // namespace cortex
