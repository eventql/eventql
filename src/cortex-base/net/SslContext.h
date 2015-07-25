// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <cortex-base/Api.h>
#include <string>
#include <openssl/ssl.h>

namespace cortex {

class SslConnector;

/**
 * An SSL context (certificate & keyfile).
 */
class CORTEX_API SslContext {
 public:
  SslContext(SslConnector* connector,
             const std::string& crtFile,
             const std::string& keyFile);
  ~SslContext();

  SSL_CTX* get() const;

  const std::vector<std::string>& dnsNames() const;

  bool isValidDnsName(const std::string& servername) const;

 private:
  static bool imatch(const std::string& pattern, const std::string& value);
  static int onServerName(SSL* ssl, int* ad, SslContext* self);
  static int onNextProtosAdvertised(SSL* ssl,
      const unsigned char** out, unsigned int* outlen, void* pself);
  static int onAppLayerProtoNegotiation(SSL* ssl,
      const unsigned char **out, unsigned char *outlen,
      const unsigned char *in, unsigned int inlen, void *pself);

 private:
  SslConnector* connector_;
  SSL_CTX* ctx_;
  std::vector<std::string> dnsNames_;
};

inline SSL_CTX* SslContext::get() const {
  return ctx_;
}

inline const std::vector<std::string>& SslContext::dnsNames() const {
  return dnsNames_;
}

// some helper
CORTEX_API const std::error_category& ssl_error_category();

} // namespace cortex


