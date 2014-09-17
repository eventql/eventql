#include <xzero/http/v1/HttpConnectionFactory.h>
#include <xzero/http/v1/HttpConnection.h>

namespace xzero {
namespace http1 {

HttpConnectionFactory::HttpConnectionFactory()
    : xzero::HttpConnectionFactory("http") {
  setInputBufferSize(16 * 1024);
}

HttpConnectionFactory::~HttpConnectionFactory() {
}

Connection* HttpConnectionFactory::create(
    Connector* connector, std::shared_ptr<EndPoint> endpoint) {
  return configure(new http1::HttpConnection(endpoint, handler(),
                                             dateGenerator()),
                   connector);
}

}  // namespace http1
}  // namespace xzero
