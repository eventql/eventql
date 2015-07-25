/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_UTIL_HMAC_H
#define _STX_UTIL_HMAC_H
#include <stx/stdtypes.h>
#include <stx/exception.h>
#include <stx/buffer.h>
#include <stx/SHA1.h>

namespace stx {

class HMAC {
public:

  const static size_t kBlockSize;
  const static char kOPad;
  const static char kIPad;
  //$blockSize=64,$opad=0x5c,$ipad=0x36

  static SHA1Hash hmac_sha1(
      const Buffer& key,
      const Buffer& message);

};

}

#endif
