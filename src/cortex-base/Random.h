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
#include <cortex-base/RuntimeError.h>
#include <string>
#include <random>

namespace cortex {

/**
 * Random Value Generator.
 */
class CORTEX_API Random {
 public:
  Random();

  /**
   * Return 64 bits of random data
   */
  uint64_t random64();

  /**
   * Return 64 bits of random data as hex chars
   */
  std::string hex64();

  /**
   * Return 128 bits of random data as hex chars
   */
  std::string hex128();

  /**
   * Return 256 bits of random data as hex chars
   */
  std::string hex256();

  /**
   * Return 512 bits of random data as hex chars
   */
  std::string hex512();

  /**
   * Create a new random alphanumeric string
   */
  std::string alphanumericString(int nchars);

 protected:
  std::mt19937_64 prng_;
};

}  // namespace cortex
