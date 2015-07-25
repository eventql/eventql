// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-http/fastcgi/bits.h>
#include <string>

namespace cortex {
namespace http {
namespace fastcgi {

CORTEX_HTTP_API std::string to_string(Type t) {
  switch (t) {
    case Type::BeginRequest: return "BeginRequest";
    case Type::AbortRequest: return "AbortRequest";
    case Type::EndRequest: return "EndRequest";
    case Type::Params: return "Params";
    case Type::StdIn: return "StdIn";
    case Type::StdOut: return "StdOut";
    case Type::StdErr: return "StdErr";
    case Type::Data: return "Data";
    case Type::GetValues: return "GetValues";
    case Type::GetValuesResult: return "GetValuesResult";
    case Type::UnknownType: return "UnknownType";
    default: return std::to_string(static_cast<int>(t));
  }
}

void CgiParamStreamReader::processParams(const char *buf,
                                         size_t length) {
  const char *i = buf;
  size_t pos = 0;

  while (pos < length) {
    size_t nameLength = 0;
    size_t valueLength = 0;

    if ((*i >> 7) == 0) {  // 11 | 14
      nameLength = *i & 0xFF;
      ++pos;
      ++i;

      if ((*i >> 7) == 0) {  // 11
        valueLength = *i & 0xFF;
        ++pos;
        ++i;
      } else {  // 14
        valueLength = (((i[0] & ~(1 << 7)) & 0xFF) << 24) +
                      ((i[1] & 0xFF) << 16) + ((i[2] & 0xFF) << 8) +
                      ((i[3] & 0xFF));
        pos += 4;
        i += 4;
      }
    } else {  // 41 || 44
      nameLength = (((i[0] & ~(1 << 7)) & 0xFF) << 24) + ((i[1] & 0xFF) << 16) +
                   ((i[2] & 0xFF) << 8) + ((i[3] & 0xFF));
      pos += 4;
      i += 4;

      if ((*i >> 7) == 0) {  // 41
        valueLength = *i;
        ++pos;
        ++i;
      } else {  // 44
        valueLength = (((i[0] & ~(1 << 7)) & 0xFF) << 24) +
                      ((i[1] & 0xFF) << 16) + ((i[2] & 0xFF) << 8) +
                      ((i[3] & 0xFF));
        pos += 4;
        i += 4;
      }
    }

    const char *name = i;
    pos += nameLength;
    i += nameLength;

    const char *value = i;
    pos += valueLength;
    i += valueLength;

    onParam(name, nameLength, value, valueLength);
  }
}

void CgiParamStreamWriter::encodeHeader(const char *name,
                                        size_t nameLength,
                                        const char *value,
                                        size_t valueLength) {
  encodeLength(5 + nameLength);
  encodeLength(valueLength);

  buffer_.push_back("HTTP_");
  for (size_t i = 0; i < nameLength; ++i) {
    const char ch = name[i];
    if (ch == '-') {
      buffer_ << '_';
    } else {
      buffer_ << std::toupper(ch);
    }
  }
  buffer_.push_back(value, valueLength);
}

void CgiParamStreamWriter::encode(
    const char *name,
    size_t nameLength,
    const char *value,
    size_t valueLength) {
  encodeLength(nameLength);
  encodeLength(valueLength);

  buffer_.push_back(name, nameLength);
  buffer_.push_back(value, valueLength);
}

void CgiParamStreamWriter::encode(
    const char *name, size_t nameLength,
    const char *v1, size_t l1,
    const char *v2, size_t l2) {
  encodeLength(nameLength);
  encodeLength(l1 + l2);

  buffer_.push_back(name, nameLength);
  buffer_.push_back(v1, l1);
  buffer_.push_back(v2, l2);
}

void CgiParamStreamWriter::encodeLength(size_t length) {
  if (length < 127) {
    buffer_.push_back(static_cast<char>(length));
  } else {
    unsigned char value[4];
    value[0] = ((length >> 24) & 0xFF) | 0x80;
    value[1] = (length >> 16) & 0xFF;
    value[2] = (length >> 8) & 0xFF;
    value[3] = length & 0xFF;

    buffer_.push_back(&value, sizeof(value));
  }
}

} // namespace fastcgi
} // namespace http
} // namespace cortex
