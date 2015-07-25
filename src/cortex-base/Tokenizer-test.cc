// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <gtest/gtest.h>
#include <cortex-base/Tokenizer.h>
#include <cortex-base/Buffer.h>

class TokenizerTest : public ::testing::Test {
 public:
  typedef cortex::Tokenizer<cortex::BufferRef, cortex::BufferRef> BufferTokenizer;

  void SetUp();
  void TearDown();

  void Tokenize3();
};

void TokenizerTest::SetUp() {}

void TokenizerTest::TearDown() {}

TEST_F(TokenizerTest, Tokenize3) {
  cortex::Buffer input("/foo/bar/com");
  BufferTokenizer st(input.ref(1), "/");
  std::vector<cortex::BufferRef> tokens = st.tokenize();

  ASSERT_EQ(3, tokens.size());
  EXPECT_EQ("foo", tokens[0]);
  EXPECT_EQ("bar", tokens[1]);
  EXPECT_EQ("com", tokens[2]);
}
