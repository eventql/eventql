#include <xzero/http/HttpOutputFilter.h>
#include <xzero/io/FileRef.h>
#include <xzero/Buffer.h>
#include <xzero/sysconfig.h>
#include <stdexcept>
#include <system_error>

namespace xzero {

void HttpOutputFilter::applyFilters(
    const std::list<std::shared_ptr<HttpOutputFilter>>& filters,
    const BufferRef& input, Buffer* output) {
  auto i = filters.begin();
  auto e = filters.end();
  (*i)->filter(input, output);
  i++;
  Buffer tmp;
  while (i != e) {
    tmp.swap(*output);
    (*i)->filter(tmp.ref(), output);
    i++;
  }
}

void HttpOutputFilter::applyFilters(
    const std::list<std::shared_ptr<HttpOutputFilter>>& filters,
    const FileRef& file, Buffer* output) {

  Buffer input;
  file.fill(&input);

  HttpOutputFilter::applyFilters(filters, input, output);
}

} // namespace xzero
