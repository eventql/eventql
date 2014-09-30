#include <xzero/http/HttpOutputFilter.h>
#include <xzero/Buffer.h>

namespace xzero {

void HttpOutputFilter::applyFilters(const std::list<HttpOutputFilter*>& filters,
                                    const BufferRef& input,
                                    Buffer* output) {
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

} // namespace xzero
