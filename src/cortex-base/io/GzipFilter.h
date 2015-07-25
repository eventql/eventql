// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/Api.h>
#include <cortex-base/io/Filter.h>
#include <cortex-base/sysconfig.h>
#include <string>

#include <zlib.h>

namespace cortex {

/**
 * Gzip encoding filter.
 */
class CORTEX_API GzipFilter : public Filter {
 public:
  explicit GzipFilter(int level);
  ~GzipFilter();

  void filter(const BufferRef& input, Buffer* output, bool last) override;

 private:
  std::string z_code(int code) const;

 private:
  z_stream z_;
};

} // namespace cortex
