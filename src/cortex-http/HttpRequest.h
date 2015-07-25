// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <cortex-http/Api.h>
#include <cortex-base/sysconfig.h>
#include <cortex-base/Buffer.h>
#include <cortex-base/io/File.h>
#include <cortex-http/HeaderFieldList.h>
#include <cortex-http/HttpVersion.h>
#include <cortex-http/HttpMethod.h>
#include <cortex-http/HttpInput.h>
#include <cortex-base/net/IPAddress.h>
#include <cortex-base/Option.h>
#include <memory>

namespace cortex {
namespace http {

/**
 * Represents an HTTP request message.
 */
class CORTEX_HTTP_API HttpRequest {
 public:
  HttpRequest();
  explicit HttpRequest(std::unique_ptr<HttpInput>&& input);
  HttpRequest(const std::string& method, const std::string& path,
              HttpVersion version, bool secure, const HeaderFieldList& headers,
              std::unique_ptr<HttpInput>&& input);

  void setRemoteIP(const Option<IPAddress>& ip);
  const Option<IPAddress>& remoteIP() const;

  size_t bytesReceived() const noexcept { return bytesReceived_; }
  void setBytesReceived(size_t n) { bytesReceived_ = n; }

  HttpMethod method() const CORTEX_NOEXCEPT { return method_; }
  const std::string& unparsedMethod() const CORTEX_NOEXCEPT { return unparsedMethod_; }
  void setMethod(const std::string& value);

  bool setUri(const std::string& uri);
  const std::string& unparsedUri() const CORTEX_NOEXCEPT { return unparsedUri_; }
  const std::string& path() const CORTEX_NOEXCEPT { return path_; }
  const std::string& query() const CORTEX_NOEXCEPT { return query_; }
  int directoryDepth() const CORTEX_NOEXCEPT { return directoryDepth_; }

  HttpVersion version() const CORTEX_NOEXCEPT { return version_; }
  void setVersion(HttpVersion version) { version_ = version; }

  const HeaderFieldList& headers() const CORTEX_NOEXCEPT { return headers_; }
  HeaderFieldList& headers() { return headers_; }

  const std::string& host() const CORTEX_NOEXCEPT { return host_; }
  void setHost(const std::string& value);

  bool isSecure() const CORTEX_NOEXCEPT { return secure_; }
  void setSecure(bool secured) { secure_ = secured; }

  HttpInput* input() const { return input_.get(); }
  void setInput(std::unique_ptr<HttpInput>&& input) { input_ = std::move(input); }

  bool expect100Continue() const CORTEX_NOEXCEPT { return expect100Continue_; }
  void setExpect100Continue(bool value) CORTEX_NOEXCEPT { expect100Continue_ = value; }

  const std::string& username() const noexcept { return username_; }
  void setUserName(const std::string& value) { username_ = value; }

  void recycle();

 private:
  Option<IPAddress> remoteIP_;
  size_t bytesReceived_;

  HttpMethod method_;
  std::string unparsedMethod_;

  std::string unparsedUri_;
  std::string path_;
  std::string query_;
  int directoryDepth_;

  HttpVersion version_;

  bool secure_;
  bool expect100Continue_;
  std::string host_;

  HeaderFieldList headers_;
  std::unique_ptr<HttpInput> input_;

  std::string username_; // the client's username, if authenticated
};

}  // namespace http
}  // namespace cortex
