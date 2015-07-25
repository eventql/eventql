// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
#pragma once

// this file holds the protocol bits

#include <stdint.h>     // uint16_t, ...
#include <string.h>     // memset
#include <arpa/inet.h>  // htons/ntohs/ntohl/htonl

#include <list>
#include <string>
#include <vector>
#include <utility>
#include <unordered_map>

#include <cortex-http/Api.h>
#include <cortex-base/Buffer.h>

namespace cortex {
namespace http {
namespace fastcgi {

enum class Type {
  BeginRequest = 1,
  AbortRequest = 2,
  EndRequest = 3,
  Params = 4,
  StdIn = 5,
  StdOut = 6,
  StdErr = 7,
  Data = 8,
  GetValues = 9,
  GetValuesResult = 10,
  UnknownType = 11
};

CORTEX_HTTP_API std::string to_string(Type t);

enum class Role {
  Responder = 1,
  Authorizer = 2,
  Filter = 3
};

enum class ProtocolStatus {
  RequestComplete = 0,
  CannotMpxConnection = 1,
  Overloaded = 2,
  UnknownRole = 3
};

struct CORTEX_PACKED Record {
 private:
  uint8_t version_;
  uint8_t type_;
  uint16_t requestId_;
  uint16_t contentLength_;
  uint8_t paddingLength_;
  uint8_t reserved_;

 public:
  Record(Type type,
         uint16_t requestId,
         uint16_t contentLength,
         uint8_t paddingLength);

  // static Record *create(Type type, uint16_t requestId, uint16_t
  // contentLength);
  // void destroy();

  int version() const { return version_; }
  Type type() const { return static_cast<Type>(type_); }
  int requestId() const { return ntohs(requestId_); }
  int contentLength() const { return ntohs(contentLength_); }
  int paddingLength() const { return paddingLength_; }
  uint8_t reserved0() const { return reserved_; }

  const char *content() const {
    return reinterpret_cast<const char *>(this) + sizeof(*this);
  }

  const char *data() const { return reinterpret_cast<const char *>(this); }
  uint32_t size() const {
    return sizeof(Record) + contentLength() + paddingLength();
  }

  bool isManagement() const { return requestId() == 0; }
  bool isApplication() const { return requestId() != 0; }

  const char *type_str() const {
    static const char *map[] = {0,            "BeginRequest",    "AbortRequest",
                                "EndRequest", "Params",          "StdIn",
                                "StdOut",     "StdErr",          "Data",
                                "GetValues",  "GetValuesResult", "UnknownType"};

    return type_ > 0 && type_ < 12 ? map[type_] : "invalid";
  }
};

struct CORTEX_PACKED BeginRequestRecord : public Record {
 private:
  uint16_t role_;
  uint8_t flags_;
  uint8_t reserved_[5];

 public:
  BeginRequestRecord(Role role, uint16_t requestId, bool keepAlive)
      : Record(Type::BeginRequest, requestId, 8, 0),
        role_(htons(static_cast<uint16_t>(role))),
        flags_(keepAlive ? 0x01 : 0x00) {
    memset(reserved_, 0, sizeof(reserved_));
  }

  Role role() const noexcept { return static_cast<Role>(ntohs(role_)); }

  bool isKeepAlive() const noexcept { return flags_ & 0x01; }

  const char *role_str() const noexcept {
    switch (role()) {
      case Role::Responder:
        return "responder";
      case Role::Authorizer:
        return "authorizer";
      case Role::Filter:
        return "filter";
      default:
        return "invalid";
    }
  }
};

/** generates a PARAM stream. */
class CgiParamStreamWriter {
 private:
  cortex::Buffer buffer_;

  inline void encodeLength(size_t length);

 public:
  CgiParamStreamWriter();

  static Buffer encode(
      const std::list<std::pair<std::string, std::string>>& params);

  /**
   * Encodes a request header and value into the PARAM stream.
   *
   * The header name is properly and effeciently prefixed with "HTTP_",
   * uppercased, and dashes replaced with underscores.
   */
  void encodeHeader(const char *name, size_t nameLength,
                    const char *value, size_t valueLength);

  /**
   * Encodes a request header and value into the PARAM stream.
   *
   * The header name is properly and effeciently prefixed with "HTTP_",
   * uppercased, and dashes replaced with underscores.
   */
  void encodeHeader(const std::string& name, const std::string& value) {
    encodeHeader(name.data(), name.size(), value.data(), value.size());
  }

  void encode(const char *name, size_t nameLength,
              const char *value, size_t valueLength);

  void encode(const char *name, size_t nameLength,
              const char *v1, size_t l1,
              const char *v2, size_t l2);

  void encode(const std::string &name, const std::string &value) {
    encode(name.data(), name.size(), value.data(), value.size());
  }

  void encode(const cortex::BufferRef &name, const cortex::BufferRef &value) {
    encode(name.data(), name.size(), value.data(), value.size());
  }

  void encode(const std::string &name, const cortex::BufferRef &value) {
    encode(name.data(), name.size(), value.data(), value.size());
  }

  template <typename V1, typename V2>
  void encode(const std::string &name, const V1 &v1, const V2 &v2) {
    encode(name.data(), name.size(), v1.data(), v1.size(), v2.data(),
           v2.size());
  }

  template <typename PodType, std::size_t N, typename ValuePType,
            std::size_t N2>
  void encode(PodType (&name)[N], const ValuePType (&value)[N2]) {
    encode(name, N - 1, value, N2 - 1);
  }

  template <typename PodType, std::size_t N, typename ValueType>
  void encode(PodType (&name)[N], const ValueType &value) {
    encode(name, N - 1, value.data(), value.size());
  }

  cortex::Buffer& output() { return buffer_; }
};

/** parses a PARAM stream and reads out name/value paris. */
class CgiParamStreamReader {
 private:
  std::vector<char> name_;   // name token
  std::vector<char> value_;  // value token

 public:
  CgiParamStreamReader();

  void processParams(const char *buf, size_t length);
  virtual void onParam(const char *name, size_t nameLen, const char *value,
                       size_t valueLen) = 0;
};

struct CORTEX_PACKED AbortRequestRecord : public Record {
 public:
  explicit AbortRequestRecord(uint16_t requestId);
};

struct CORTEX_PACKED UnknownTypeRecord : public Record {
 private:
  uint8_t unknownType_;
  uint8_t reserved_[7];

 public:
  UnknownTypeRecord(Type type, uint16_t requestId);

  int unknownType() const noexcept;
};

struct CORTEX_PACKED EndRequestRecord : public Record {
 private:
  uint32_t appStatus_;
  uint8_t protocolStatus_;
  uint8_t reserved_[3];

 public:
  EndRequestRecord(uint16_t requestId,
                   uint32_t appStatus,
                   ProtocolStatus protocolStatus);

  uint32_t appStatus() const;
  ProtocolStatus protocolStatus() const;
};

} // namespace fastcgi
} // namespace http
} // namespace cortex

#include <cortex-http/fastcgi/bits-inl.h>
