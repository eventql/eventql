// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#ifndef sw_x0_Base64_h
#define sw_x0_Base64_h (1)

#include <cortex-base/Api.h>
#include <cortex-base/Buffer.h>

namespace cortex {

/**
 * @brief Implements a Base64 encoder/decoder.
 */
class CORTEX_API Base64 {
 private:
  static const char base64_[];
  static const unsigned char pr2six_[256];

 public:
  /**
   * @brief computes the buffer length of the target buffer.
   */
  static int encodeLength(int sourceLength);

  static std::string encode(const std::string& text);
  static std::string encode(const Buffer& buffer);
  static std::string encode(const unsigned char* buffer, int length);

  /**
   * @brief compuutes decoding buffer size of given base64-buffer.
   */
  static int decodeLength(const std::string& buffer);

  /**
   * @brief compuutes decoding buffer size of given base64-buffer.
   */
  static int decodeLength(const char* buffer);

  static Buffer decode(const std::string& base64Value);
  static int decode(const char* input, unsigned char* output);
  static Buffer decode(const BufferRef& base64Value);
};

}  // namespace cortex

#endif
