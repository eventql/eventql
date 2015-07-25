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
#include <functional>
#include <string>
#include <memory>

namespace cortex {

class File;

class CORTEX_API FileRepository {
 public:
  virtual ~FileRepository();

  virtual std::shared_ptr<File> getFile(
      const std::string& requestPath,
      const std::string& docroot = "/") = 0;

  virtual void listFiles(std::function<bool(const std::string&)> callback) = 0;
  virtual void deleteAllFiles() = 0;

  virtual int createTempFile(std::string* filename = nullptr) = 0;
};

}  // namespace cortex
