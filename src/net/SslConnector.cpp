// This file is part of the "x0" project
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/net/SslConnector.h>
#include <xzero/sysconfig.h>
#include <xzero/RuntimeError.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/tls1.h>

namespace xzero {

#define THROW_SSL_ERROR() { \
  char buf[256]; \
  ERR_error_string_n(ERR_get_error(), buf, sizeof(buf)); \
  throw RUNTIME_ERROR(buf); \
}

static inline SSL_CTX* createSslContext() {
  SSL_library_init();
  SSL_load_error_strings();
  OpenSSL_add_all_algorithms();

  const SSL_METHOD* method = TLSv1_2_server_method();
  SSL_CTX* ctx = SSL_CTX_new(method);
  if (ctx == NULL)
    THROW_SSL_ERROR();

  return ctx;
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
  SSL_CTX* ctx = createSslContext();

  if (SSL_CTX_use_certificate_file(ctx, crtFilePath.c_str(), SSL_FILETYPE_PEM) <= 0)
    THROW_SSL_ERROR();

  if (SSL_CTX_use_PrivateKey_file(ctx, keyFilePath.c_str(), SSL_FILETYPE_PEM) <= 0)
    THROW_SSL_ERROR();

  if (!SSL_CTX_check_private_key(ctx))
    throw RUNTIME_ERROR("Private key does not match the public certificate");

  contexts_.push_back(ctx);

  // see http://stackoverflow.com/a/5113466/386670
  // SSL_CTX_set_tlsext_servername_callback()
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

} // namespace xzero
