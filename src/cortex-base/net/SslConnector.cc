// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/net/SslConnector.h>
#include <cortex-base/net/SslContext.h>
#include <cortex-base/net/Connection.h>
#include <cortex-base/sysconfig.h>
#include <cortex-base/RuntimeError.h>
#include <openssl/ssl.h>
#include <algorithm>

namespace cortex {

#ifndef NDEBUG
#define TRACE(msg...) { \
  fprintf(stderr, "SslConnector: " msg); \
  fprintf(stderr, "\n"); \
  fflush(stderr); \
}
#else
#define TRACE(msg...) do {} while (0)
#endif

SslConnector::SslConnector(const std::string& name, Executor* executor,
                           Scheduler* scheduler, WallClock* clock,
                           TimeSpan readTimeout, TimeSpan writeTimeout,
                           TimeSpan tcpFinTimeout,
                           std::function<void(const std::exception&)> eh,
                           const IPAddress& ipaddress, int port, int backlog,
                           bool reuseAddr, bool reusePort)
    : InetConnector(name, executor, scheduler, clock,
                    readTimeout, writeTimeout, tcpFinTimeout, eh,
                    ipaddress, port, backlog, reuseAddr, reusePort),
      contexts_() {
}

SslConnector::~SslConnector() {
}

void SslConnector::addContext(const std::string& crtFilePath,
                              const std::string& keyFilePath) {
  contexts_.emplace_back(new SslContext(this, crtFilePath, keyFilePath));
}

SslContext* SslConnector::selectContext(const char* servername) const {
  TRACE("%p selectContext: servername = '%s'", this, servername);
  if (!servername)
    return nullptr;

  for (const auto& ctx: contexts_)
    if (ctx->isValidDnsName(servername))
      return ctx.get();

  return nullptr;
}

int SslConnector::selectContext(
    SSL* ssl, int* ad, SslConnector* self) {
  const char * servername = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
  TRACE("%p selectContext: servername = '%s'", self, servername);

  if (!servername)
    return SSL_TLSEXT_ERR_NOACK;

  for (const auto& ctx: self->contexts_) {
    if (ctx->isValidDnsName(servername)) {
      TRACE("selecting context %p\n", ctx->get());
      SSL_set_SSL_CTX(ssl, ctx->get());
      return SSL_TLSEXT_ERR_OK;
    }
  }

  TRACE("using default context %p", SSL_get_SSL_CTX(ssl));
  return SSL_TLSEXT_ERR_OK;
}

void SslConnector::start() {
  InetConnector::start();
}

bool SslConnector::isStarted() const CORTEX_NOEXCEPT {
  return InetConnector::isStarted();
}

void SslConnector::stop() {
  InetConnector::stop();
}

std::list<RefPtr<EndPoint>> SslConnector::connectedEndPoints() {
  return InetConnector::connectedEndPoints();
}

RefPtr<EndPoint> SslConnector::createEndPoint(int cfd) {
  return make_ref<SslEndPoint>(cfd, this, scheduler()).as<EndPoint>();
}

void SslConnector::onEndPointCreated(const RefPtr<EndPoint>& endpoint) {
  endpoint.weak_as<SslEndPoint>()->onHandshake();
}

} // namespace cortex
