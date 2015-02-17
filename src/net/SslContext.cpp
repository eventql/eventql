// This file is part of the "x0" project
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/net/SslConnector.h>
#include <xzero/net/SslContext.h>
#include <xzero/net/Connection.h>
#include <xzero/sysconfig.h>
#include <xzero/RuntimeError.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/tls1.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/objects.h>
#include <algorithm>

namespace xzero {

#ifndef NDEBUG
#define TRACE(msg...) { \
  fprintf(stderr, "SslContext: " msg); \
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

// {{{ helper
static inline void initializeSslLibrary() {
  static int initCounter = 0;
  if (initCounter == 0) {
    initCounter++;
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
  }
}

static std::vector<std::string> collectDnsNames(X509* crt) {
  std::vector<std::string> result;

  auto addUnique = [&](const std::string& name) {
    auto i = std::find(result.begin(), result.end(), name);
    if (i == result.end()) {
      result.push_back(name);
    }
  };

  // retrieve DNS-Name extension entries
  STACK_OF(GENERAL_NAME)* altnames =
      (STACK_OF(GENERAL_NAME)*) X509_get_ext_d2i(crt, NID_subject_alt_name,
                                                 nullptr, nullptr);
  int numAltNames = sk_GENERAL_NAME_num(altnames);

  for (int i = 0; i < numAltNames; ++i) {
    GENERAL_NAME* altname = sk_GENERAL_NAME_value(altnames, i);
    if (altname->type == GEN_DNS) {
      std::string dnsName(reinterpret_cast<char*>(altname->d.dNSName->data),
                          altname->d.dNSName->length);
      addUnique(dnsName);
    }
  }
  GENERAL_NAMES_free(altnames);

  // Retrieve "commonName" subject
  X509_NAME* sname = X509_get_subject_name(crt);
  if (sname) {
    int i = -1;
    for (;;) {
      i = X509_NAME_get_index_by_NID(sname, NID_commonName, i);
      if (i < 0)
        break;

      X509_NAME_ENTRY* entry = X509_NAME_get_entry(sname, i);
      ASN1_STRING* str = X509_NAME_ENTRY_get_data(entry);

      std::string cn((char*)ASN1_STRING_data(str), ASN1_STRING_length(str));

      addUnique(cn);
    }
  }

  return result;
}

static std::vector<std::string> collectDnsNames(SSL_CTX* ctx) {
  SSL* ssl = SSL_new(ctx);
  X509* crt = SSL_get_certificate(ssl);
  std::vector<std::string> result = collectDnsNames(crt);
  SSL_free(ssl);
  return result;
}
// }}}

SslContext::SslContext(SslConnector* connector,
                       const std::string& crtFilePath,
                       const std::string& keyFilePath) {
  TRACE("%p SslContext(\"%s\", \"%s\"", this, crtFilePath.c_str(), keyFilePath.c_str());

  connector_ = connector;

  initializeSslLibrary();

  ctx_ = SSL_CTX_new(TLSv1_2_server_method());
  if (!ctx_)
    THROW_SSL_ERROR();

  if (SSL_CTX_use_certificate_chain_file(ctx_, crtFilePath.c_str()) <= 0)
    THROW_SSL_ERROR();

  if (SSL_CTX_use_PrivateKey_file(ctx_, keyFilePath.c_str(), SSL_FILETYPE_PEM) <= 0)
    THROW_SSL_ERROR();

  if (!SSL_CTX_check_private_key(ctx_))
    throw RUNTIME_ERROR("Private key does not match the public certificate");

  SSL_CTX_set_tlsext_servername_callback(ctx_, &SslContext::onServerName);
  SSL_CTX_set_tlsext_servername_arg(ctx_, this);

#ifdef TLSEXT_TYPE_application_layer_protocol_negotiation
  SSL_CTX_set_alpn_select_cb(ctx_, &SslContext::onAppLayerProtoNegotiation, this);
#endif

#ifdef TLSEXT_TYPE_next_proto_neg
  SSL_CTX_set_next_protos_advertised_cb(ctx_, &SslContext::onNextProtosAdvertised, this);
#endif

  dnsNames_ = collectDnsNames(ctx_);
}

SslContext::~SslContext() {
  TRACE("%p ~SslContext()", this);
  SSL_CTX_free(ctx_);
}

#define NPN_HTTP_1_1 "\x08http/1.1"
#define NPN_BLAH_1_0 "\x08" "blah/1.0"

int SslContext::onAppLayerProtoNegotiation(SSL* ssl,
    const unsigned char **out, unsigned char *outlen,
    const unsigned char *in, unsigned int inlen, void *pself) {
#ifdef TLSEXT_TYPE_application_layer_protocol_negotiation
  TRACE("SSL ALPN callback");

  for (int i = 0; i < inlen; i += in[i] + 1) {
    std::string proto((char*)&in[i + 1], in[i]);
    printf("SSL ALPN client support [%d]: (%d) %s\n", i, in[i], proto.c_str());
  }

  const unsigned char* srv = (const unsigned char*) NPN_HTTP_1_1;
  unsigned int srvlen = sizeof(NPN_HTTP_1_1) - 1;

  int rv = SSL_select_next_proto(
      (unsigned char**) out, outlen,
      srv, srvlen,
      in, inlen);

  if (rv != OPENSSL_NPN_NEGOTIATED)
    return SSL_TLSEXT_ERR_NOACK;

  TRACE("SSL ALPN selected: \"%*s\"", *outlen, *out);

  return SSL_TLSEXT_ERR_OK;
#else
  return SSL_TLSEXT_ERR_NOACK;
#endif
}

/**
 * NPN-callback invoked to inform the client what next-protocols the
 * server supports.
 */
int SslContext::onNextProtosAdvertised(SSL* ssl,
    const unsigned char** out, unsigned int* outlen, void* pself) {
#ifdef TLSEXT_TYPE_next_proto_neg
  TRACE("%p NPN callback", pself);

  *out = (const unsigned char*) NPN_BLAH_1_0 NPN_HTTP_1_1;
  *outlen = sizeof(NPN_BLAH_1_0 NPN_HTTP_1_1) - 1;

  return SSL_TLSEXT_ERR_OK;
#else
  return SSL_TLSEXT_ERR_NOACK;
#endif
}

int SslContext::onServerName(SSL* ssl, int* ad, SslContext* self) {
  TRACE("%p onServerName()", self);
  const char* name = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);

  if (SslContext* ctx = self->connector_->selectContext(name)) {
    SSL_set_SSL_CTX(ssl, ctx->get());
    return SSL_TLSEXT_ERR_OK;
  }

  return SSL_TLSEXT_ERR_NOACK;
}

bool SslContext::isValidDnsName(const std::string& servername) const {
  for (const std::string& pattern: dnsNames_)
    if (imatch(pattern, servername))
      return true;

  return false;
}

bool SslContext::imatch(const std::string& pattern, const std::string& value) {
  int s = pattern.size() - 1;
  int t = value.size() - 1;

  for (; s > 0 && t > 0 && pattern[s] == value[t]; --s, --t)
    ;

  if (!s && !t)
    return true;

  if (pattern[s] == '*')
    return true;

  return false;
}

} // namespace xzero
