// TODO

#include <cortex-flow/FlowLexer.h>
#include <cortex-flow/FlowToken.h>
#include <gtest/gtest.h>

using namespace cortex;
using namespace cortex::flow;

TEST(FlowLexer, eof) {
  FlowLexer lexer;
  lexer.openString("");

  ASSERT_EQ(FlowToken::Eof, lexer.token());
  ASSERT_EQ(FlowToken::Eof, lexer.nextToken());
  ASSERT_EQ(FlowToken::Eof, lexer.token());
  ASSERT_EQ(1, lexer.line());
  ASSERT_EQ(1, lexer.column());
}

TEST(FlowLexer, token_keywords) {
  FlowLexer lexer;
  lexer.openString("handler");

  ASSERT_EQ(FlowToken::Handler, lexer.token());
  ASSERT_EQ(1, lexer.line());
  ASSERT_EQ(7, lexer.column());
}

TEST(FlowLexer, composed) {
  FlowLexer lexer;
  lexer.openString("handler main {}");

  ASSERT_EQ(FlowToken::Handler, lexer.token());
  ASSERT_EQ("handler", lexer.stringValue());

  ASSERT_EQ(FlowToken::Ident, lexer.nextToken());
  ASSERT_EQ("main", lexer.stringValue());

  ASSERT_EQ(FlowToken::Begin, lexer.nextToken());

  ASSERT_EQ(FlowToken::End, lexer.nextToken());

  ASSERT_EQ(FlowToken::Eof, lexer.nextToken());

  ASSERT_EQ(FlowToken::Eof, lexer.nextToken());
}
