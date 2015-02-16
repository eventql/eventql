// This file is part of the "x0" project
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/net/SslConnector.h>
#include <xzero/net/Connection.h>
#include <xzero/sysconfig.h>
#include <xzero/RuntimeError.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/tls1.h>

namespace xzero {

#ifndef NDEBUG
#define TRACE(msg...) { \
  fprintf(stderr, "SslConnector: " msg); \
  fprintf(stderr, "\n"); \
  fflush(stderr); \
}
#else
#define TRACE(msg...) do {} while (0)
#endif

#define THROW_SSL_ERROR() { \
  char buf[256]; \
  ERR_error_string_n(ERR_get_error(), buf, sizeof(buf)); \
  throw RUNTIME_ERROR(buf); \
}

static inline void initializeSslLibrary() {
  static int initCounter = 0;
  if (initCounter == 0) {
    initCounter++;
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
  }
}

SslConnector::SslConnector(const std::string& name, Executor* executor,
                           Scheduler* scheduler, WallClock* clock,
                           TimeSpan idleTimeout,
                           std::function<void(const std::exception&)> eh,
                           const IPAddress& ipaddress, int port, int backlog,
                           bool reuseAddr, bool reusePort)
    : InetConnector(name, executor, scheduler, clock, idleTimeout, eh,
                    ipaddress, port, backlog, reuseAddr, reusePort),
      contexts_() {
}

SslConnector::~SslConnector() {
  for (SSL_CTX* ctx: contexts_)
    SSL_CTX_free(ctx);
}

void SslConnector::addContext(const std::string& crtFilePath,
                              const std::string& keyFilePath) {
  initializeSslLibrary();

  SSL_CTX* ctx = SSL_CTX_new(TLSv1_2_server_method());
  if (ctx == NULL)
    THROW_SSL_ERROR();

  try {
    if (SSL_CTX_use_certificate_file(ctx, crtFilePath.c_str(), SSL_FILETYPE_PEM) <= 0)
      THROW_SSL_ERROR();

    if (SSL_CTX_use_PrivateKey_file(ctx, keyFilePath.c_str(), SSL_FILETYPE_PEM) <= 0)
      THROW_SSL_ERROR();

    if (!SSL_CTX_check_private_key(ctx))
      throw RUNTIME_ERROR("Private key does not match the public certificate");

    // TODO: read CommonName & Alternative-Name (dnsname) fields

    SSL_CTX_set_tlsext_servername_callback(ctx, &SslConnector::tls1_servername_cb);
    SSL_CTX_set_tlsext_servername_arg(ctx, this);

    contexts_.push_back(ctx);
  } catch (...) {
    SSL_CTX_free(ctx);
    throw;
  }
}

int SslConnector::tls1_servername_cb(
    SSL* ssl, int* ad, SslConnector* self) {
  const char * servername = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
  TRACE("%p tls1_servername_cb: servername = '%s'", self, servername);

  if (!servername)
    return SSL_TLSEXT_ERR_NOACK;

  // TODO search for associated CTX by hostname
  SSL_CTX* newctx = self->defaultContext();

  if (newctx != SSL_get_SSL_CTX(ssl)) {
    SSL_set_SSL_CTX(ssl, self->contexts_.front());
  }

  return SSL_TLSEXT_ERR_OK;
}

void SslConnector::start() {
  InetConnector::start();
}

bool SslConnector::isStarted() const XZERO_NOEXCEPT {
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

} // namespace xzero
