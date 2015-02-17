// This file is part of the "x0" project
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/net/SslEndPoint.h>
#include <xzero/net/SslContext.h>
#include <xzero/net/SslConnector.h>
#include <xzero/net/Connection.h>
#include <xzero/net/ConnectionFactory.h>
#include <xzero/executor/Scheduler.h>
#include <xzero/RuntimeError.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <fcntl.h>
#include <unistd.h>

namespace xzero {

#ifndef NDEBUG
#define TRACE(msg...) { \
  fprintf(stderr, "SslEndPoint: " msg); \
  fprintf(stderr, "\n"); \
  fflush(stderr); \
}
#else
#define TRACE(msg...) do {} while (0)
#endif

static inline std::string sslErrorString() {
  char buf[256];
  ERR_error_string_n(ERR_get_error(), buf, sizeof(buf));
  return buf;
}

SslEndPoint::SslEndPoint(
    int socket, SslConnector* connector, Scheduler* scheduler)
    : handle_(socket),
      isCorking_(false),
      connector_(connector),
      scheduler_(scheduler),
      ssl_(nullptr),
      bioDesire_(Desire::None),
      io_(),
      idleTimeout_(connector->clock(), scheduler) {
  TRACE("%p SslEndPoint() ctor", this);

  idleTimeout_.setCallback(std::bind(&SslEndPoint::onTimeout, this));
  idleTimeout_.setTimeout(connector->idleTimeout());

  ssl_ = SSL_new(connector->defaultContext()->get());
  SSL_set_fd(ssl_, socket);

#if !defined(NDEBUG)
  SSL_set_tlsext_debug_callback(ssl_, &SslEndPoint::tlsext_debug_cb);
  SSL_set_tlsext_debug_arg(ssl_, this);
#endif
}

SslEndPoint::~SslEndPoint() {
  TRACE("%p ~SslEndPoint() dtor", this);

  SSL_free(ssl_);
  ::close(handle());
}

bool SslEndPoint::isOpen() const {
  return SSL_get_shutdown(ssl_) == 0;
}

void SslEndPoint::close() {
  if (!isOpen())
    return;

  shutdown();
}

void SslEndPoint::shutdown() {
  int rv = SSL_shutdown(ssl_);

  TRACE("%p close: SSL_shutdown -> %d", this, rv);
  if (rv == 1) {
    connector_->onEndPointClosed(this);
  } else if (rv == 0) {
    // call again
    shutdown();
  } else {
    switch (SSL_get_error(ssl_, rv)) {
      case SSL_ERROR_WANT_READ:
        io_ = scheduler_->executeOnReadable(
            handle(),
            std::bind(&SslEndPoint::shutdown, this));
        break;
      case SSL_ERROR_WANT_WRITE:
        io_ = scheduler_->executeOnWritable(
            handle(),
            std::bind(&SslEndPoint::shutdown, this));
        break;
      default:
        throw RUNTIME_ERROR("SSL shutdown error. " + sslErrorString());
    }
  }
}

void SslEndPoint::abort() {
#if 0
  // pretend we did a full shutdown
  SSL_set_shutdown(ssl_, SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN);
  shutdown();
#else
  connector_->onEndPointClosed(this);
#endif
}

size_t SslEndPoint::fill(Buffer* sink) {
  int space = sink->capacity() - sink->size();
  if (space < 4 * 1024) {
    sink->reserve(sink->capacity() + 8 * 1024);
    space = sink->capacity() - sink->size();
  }

  int rv = SSL_read(ssl_, sink->end(), space);
  if (rv > 0) {
    TRACE("%p fill(Buffer:%d) -> %d", this, space, rv);
    bioDesire_ = Desire::None;
    sink->resize(sink->size() + rv);
    return rv;
  }

  switch (SSL_get_error(ssl_, rv)) {
    case SSL_ERROR_WANT_READ:
      TRACE("%p fill(Buffer:%d) -> want read", this, space);
      bioDesire_ = Desire::Read;
      break;
    case SSL_ERROR_WANT_WRITE:
      TRACE("%p fill(Buffer:%d) -> want write", this, space);
      bioDesire_ = Desire::Write;
      break;
    case SSL_ERROR_ZERO_RETURN:
      TRACE("%p fill(Buffer:%d) -> remote endpoint closed", this, space);
      abort();
      break;
    default:
      TRACE("%p fill(Buffer:%d): SSL_read() -> %d",
          this, space, SSL_get_error(ssl_, rv));
      throw RUNTIME_ERROR("SSL read error. " + sslErrorString());
  }
  errno = EAGAIN;
  return 0;
}

size_t SslEndPoint::flush(const BufferRef& source) {
  int rv = SSL_write(ssl_, source.data(), source.size());
  if (rv > 0) {
    bioDesire_ = Desire::None;
    TRACE("%p flush(BufferRef, @%p, %d/%zu bytes)",
          this, source.data(), rv, source.size());

    return rv;
  }

  switch (SSL_get_error(ssl_, rv)) {
    case SSL_ERROR_WANT_READ:
      TRACE("%p flush(BufferRef, @%p, %zu bytes) failed -> want read.", this, source.data(), source.size());
      bioDesire_ = Desire::Read;
      break;
    case SSL_ERROR_WANT_WRITE:
      TRACE("%p flush(BufferRef, @%p, %zu bytes) failed -> want write.", this, source.data(), source.size());
      bioDesire_ = Desire::Read;
      break;
    case SSL_ERROR_ZERO_RETURN:
      TRACE("%p flush(BufferRef, @%p, %zu bytes) failed -> remote endpoint closed.", this, source.data(), source.size());
      abort();
      break;
    default:
      TRACE("%p flush(BufferRef, @%p, %zu bytes) failed. error.", this, source.data(), source.size());
      throw RUNTIME_ERROR("SSL write error. " + sslErrorString());
  }
  errno = EAGAIN;
  return 0;
}

size_t SslEndPoint::flush(int fd, off_t offset, size_t size) {
  Buffer buf;
  buf.reserve(size);
  ssize_t rv = ::read(fd, buf.data(), buf.capacity());
  if (rv < 0) {
    switch (errno) {
      case EBUSY:
      case EAGAIN:
#if defined(EWOULDBLOCK) && (EWOULDBLOCK != EAGAIN)
      case EWOULDBLOCK:
#endif
        return 0;
      default:
        throw SYSTEM_ERROR(errno);
    }
  }

  buf.resize(static_cast<size_t>(rv));
  return flush(buf);
}

void SslEndPoint::wantFill() {
  if (io_) {
    TRACE("%p wantFill: ignored due to active io", this);
    return;
  }

  switch (bioDesire_) {
    case Desire::None:
    case Desire::Read:
      TRACE("%p wantFill: read", this);
      io_ = scheduler_->executeOnReadable(
          handle(),
          std::bind(&SslEndPoint::fillable, this));
      break;
    case Desire::Write:
      TRACE("%p wantFill: write", this);
      io_ = scheduler_->executeOnWritable(
          handle(),
          std::bind(&SslEndPoint::fillable, this));
      break;
  }
}

void SslEndPoint::fillable() {
  TRACE("%p fillable()", this);
  RefPtr<EndPoint> _guard(this);
  try {
    io_.reset();
    bioDesire_ = Desire::None;
    connection()->onFillable();
  } catch (const std::exception& e) {
    connection()->onInterestFailure(e);
  } catch (...) {
    connection()->onInterestFailure(
        RUNTIME_ERROR("Unhandled unknown exception caught."));
  }
}

void SslEndPoint::wantFlush() {
  if (io_) {
    TRACE("%p wantFlush: ignored due to active io", this);
    return;
  }

  switch (bioDesire_) {
    case Desire::Read:
      TRACE("%p wantFlush: read", this);
      io_ = scheduler_->executeOnReadable(
          handle(),
          std::bind(&SslEndPoint::flushable, this));
      break;
    case Desire::None:
    case Desire::Write:
      TRACE("%p wantFlush: write", this);
      io_ = scheduler_->executeOnWritable(
          handle(),
          std::bind(&SslEndPoint::flushable, this));
      break;
  }
}

TimeSpan SslEndPoint::idleTimeout() {
  return idleTimeout_.timeout();
}

void SslEndPoint::setIdleTimeout(TimeSpan timeout) {
  idleTimeout_.setTimeout(timeout);
}

bool SslEndPoint::isBlocking() const {
  return !(fcntl(handle(), F_GETFL) & O_NONBLOCK);
}

void SslEndPoint::setBlocking(bool enable) {
  TRACE("%p setBlocking(\"%s\")", this, enable ? "blocking" : "nonblocking");
#if 1
  unsigned current = fcntl(handle(), F_GETFL);
  unsigned flags = enable ? (current & ~O_NONBLOCK)
                          : (current | O_NONBLOCK);
#else
  unsigned flags = enable ? fcntl(handle(), F_GETFL) & ~O_NONBLOCK
                          : fcntl(handle(), F_GETFL) | O_NONBLOCK;
#endif

  if (fcntl(handle(), F_SETFL, flags) < 0) {
    throw SYSTEM_ERROR(errno);
  }
}

bool SslEndPoint::isCorking() const {
  return isCorking_;
}

void SslEndPoint::setCorking(bool enable) {
#if defined(TCP_CORK)
  if (isCorking_ != enable) {
    int flag = enable ? 1 : 0;
    if (setsockopt(handle(), IPPROTO_TCP, TCP_CORK, &flag, sizeof(flag)) < 0)
      throw SYSTEM_ERROR(errno);

    isCorking_ = enable;
  }
#endif
}

std::string SslEndPoint::toString() const {
  char buf[64];
  int n = snprintf(buf, sizeof(buf), "SslEndPoint(fd=%d)", handle());
  return std::string(buf, n);
}

void SslEndPoint::onHandshake() {
  TRACE("%p onHandshake begin...", this);
  int rv = SSL_accept(ssl_);
  if (rv <= 0) {
    switch (SSL_get_error(ssl_, rv)) {
      case SSL_ERROR_WANT_READ:
        TRACE("%p onHandshake (want read)", this);
        scheduler_->executeOnReadable(
            handle(), std::bind(&SslEndPoint::onHandshake, this));
        break;
      case SSL_ERROR_WANT_WRITE:
        TRACE("%p onHandshake (want write)", this);
        scheduler_->executeOnWritable(
            handle(), std::bind(&SslEndPoint::onHandshake, this));
        break;
      default: {
        TRACE("%p onHandshake (error)", this);
        char buf[256];
        ERR_error_string_n(ERR_get_error(), buf, sizeof(buf));
        throw RUNTIME_ERROR(buf);
      }
    }
  } else {
    // create associated Connection object and run it
    bioDesire_ = Desire::None;
    RefPtr<EndPoint> _guard(this);
    TRACE("%p handshake complete (next protocol: \"%s\")", this, nextProtocolNegotiated().str().c_str());

    std::string protocol = nextProtocolNegotiated().str();
    auto factory = connector_->connectionFactory(protocol);
    if (!factory) {
      TRACE("%p using connection factory: default (\"%s\")", this, factory->protocolName().c_str());
      factory = connector_->defaultConnectionFactory();
    } else {
      TRACE("%p using connection factory: \"%s\"", this, factory->protocolName().c_str());
    }
    factory->create(connector_, this);

    connection()->onOpen();
  }
}

void SslEndPoint::flushable() {
  TRACE("%p flushable()", this);
  RefPtr<EndPoint> _guard(this);
  try {
    io_.reset();
    bioDesire_ = Desire::None;
    connection()->onFlushable();
  } catch (const std::exception& e) {
    connection()->onInterestFailure(e);
  } catch (...) {
    connection()->onInterestFailure(
        RUNTIME_ERROR("Unhandled unknown exception caught."));
  }
}

void SslEndPoint::onTimeout() {
  if (connection()) {
    bool finalize = connection()->onReadTimeout();

    if (finalize) {
      this->abort();
    }
  }
}

BufferRef SslEndPoint::nextProtocolNegotiated() const {
  const unsigned char* data = nullptr;
  unsigned int len = 0;

#ifdef TLSEXT_TYPE_application_layer_protocol_negotiation // ALPN
  SSL_get0_alpn_selected(ssl_, &data, &len);
  if (len > 0) {
    return BufferRef((const char*) data, len);
  }
#endif

#ifdef TLSEXT_TYPE_next_proto_neg // NPN
  SSL_get0_next_proto_negotiated(ssl_, &data, &len);
  if (len > 0) {
    return BufferRef((const char*) data, len);
  }
#endif

  return BufferRef();
}

#if !defined(NDEBUG)
static inline std::string tlsext_type_to_string(int type) {
  switch (type) {
    case TLSEXT_TYPE_server_name: return "server name";
    case TLSEXT_TYPE_max_fragment_length: return "max fragment length";
    case TLSEXT_TYPE_client_certificate_url: return "client certificate url";
    case TLSEXT_TYPE_trusted_ca_keys: return "trusted ca keys";
    case TLSEXT_TYPE_truncated_hmac: return "truncated hmac";
    case TLSEXT_TYPE_status_request: return "status request";
    case TLSEXT_TYPE_user_mapping: return "user mapping";
    case TLSEXT_TYPE_client_authz: return "client authz";
    case TLSEXT_TYPE_server_authz: return "server authz";
    case TLSEXT_TYPE_cert_type: return "cert type";
    case TLSEXT_TYPE_elliptic_curves: return "elliptic curves";
    case TLSEXT_TYPE_ec_point_formats: return "EC point formats";
    case TLSEXT_TYPE_srp: return "SRP";
    case TLSEXT_TYPE_signature_algorithms: return "signature algorithms";
    case TLSEXT_TYPE_use_srtp: return "use SRTP";
    case TLSEXT_TYPE_heartbeat: return "heartbeat";
    case TLSEXT_TYPE_session_ticket: return "session ticket";
    case TLSEXT_TYPE_renegotiate: return "renegotiate";
#if defined(TLSEXT_TYPE_opaque_prf_input)
    case TLSEXT_TYPE_opaque_prf_input: return "opaque PRF input";
#endif
#if defined(TLSEXT_TYPE_next_proto_neg) // NPN
    case TLSEXT_TYPE_next_proto_neg: return "next protocol negotiation";
#endif
#ifdef TLSEXT_TYPE_application_layer_protocol_negotiation // ALPN
    case TLSEXT_TYPE_application_layer_protocol_negotiation: return "Application Layer Protocol Negotiation";
#endif
#if defined(TLSEXT_TYPE_padding)
    case TLSEXT_TYPE_padding: return "padding";
#endif
    default: return "UNKNOWN";
  }
}

void SslEndPoint::tlsext_debug_cb(
    SSL* ssl, int client_server, int type,
    unsigned char* data, int len, SslEndPoint* self) {
  TRACE("%p TLS %s extension \"%s\" (id=%d), len=%d",
        self,
        client_server ? "server" : "client",
        tlsext_type_to_string(type).c_str(),
        type,
        len);
}
#endif // !NDEBUG

} // namespace xzero
