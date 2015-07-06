/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_UTIL_HMAC_H
#define _FNORD_UTIL_HMAC_H
#include <fnord/stdtypes.h>
#include <fnord/exception.h>
#include <fnord/buffer.h>
#include <fnord/SHA1.h>

namespace fnord {

class HMAC {
public:

  const static size_t kBlocKSize;
  const static char kOPad;
  const static char kIPad;
  //$blockSize=64,$opad=0x5c,$ipad=0x36

  static SHA1Hash hmac_sha1(
      const Buffer& key,
      const Buffer& message);

};

}

#endif
