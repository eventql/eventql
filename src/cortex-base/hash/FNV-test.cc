// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <gtest/gtest.h>
#include <cortex-base/hash/FNV.h>

TEST(FNV, FNV64) {
  cortex::hash::FNV<uint64_t> fnv64;
  EXPECT_EQ(0xE4D8CB6A3646310, fnv64.hash("fnord"));
}

TEST(FNV, FNV32) {
  cortex::hash::FNV<uint32_t> fnv32;
  EXPECT_EQ(0x6D964EB0, fnv32.hash("fnord"));
}
