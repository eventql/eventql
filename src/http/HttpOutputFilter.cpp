#include <xzero/http/HttpOutputFilter.h>
#include <xzero/io/FileRef.h>
#include <xzero/Buffer.h>
#include <xzero/sysconfig.h>
#include <stdexcept>
#include <system_error>

namespace xzero {

void HttpOutputFilter::applyFilters(
    const std::list<std::shared_ptr<HttpOutputFilter>>& filters,
    const BufferRef& input, Buffer* output, bool last) {
  auto i = filters.begin();
  auto e = filters.end();

  if (i == e) {
    *output = input;
    return;
  }

  (*i)->filter(input, output, last);
  i++;
  Buffer tmp;
  while (i != e) {
    tmp.swap(*output);
    (*i)->filter(tmp.ref(), output, last);
    i++;
  }
}

void HttpOutputFilter::applyFilters(
    const std::list<std::shared_ptr<HttpOutputFilter>>& filters,
    const FileRef& file, Buffer* output, bool last) {

  Buffer input;
  file.fill(&input);

  if (input.size() != file.size())
    throw std::runtime_error("Could not read full input file.");

  HttpOutputFilter::applyFilters(filters, input, output, last);
}

} // namespace xzero
