#include <cortex-flow/FlowParser.h>
#include <gtest/gtest.h>
#include <memory>

using namespace cortex;
using namespace cortex::flow;

TEST(FlowParser, handlerDecl) {
  auto parser = std::make_shared<FlowParser>(nullptr, nullptr, nullptr);
  parser->openString("handler main {}");
  std::unique_ptr<Unit> unit = parser->parse();

  auto h = unit->findHandler("main");
  ASSERT_TRUE(h != nullptr);
}

// TEST(FlowParser, varDecl) {} // TODO
// TEST(FlowParser, logicExpr) {} // TODO
// TEST(FlowParser, notExpr) {} // TODO
// TEST(FlowParser, relExpr) {} // TODO
// TEST(FlowParser, addExpr) {} // TODO
// TEST(FlowParser, bitNotExpr) {} // TODO
// TEST(FlowParser, primaryExpr) {} // TODO
// TEST(FlowParser, arrayExpr) {} // TODO
// TEST(FlowParser, literalExpr) {} // TODO
// TEST(FlowParser, interpolatedStringExpr) {} // TODO
// TEST(FlowParser, castExpr) {} // TODO
// TEST(FlowParser, ifStmt) {} // TODO
// TEST(FlowParser, matchStmt) {} // TODO
// TEST(FlowParser, compoundStmt) {} // TODO
// TEST(FlowParser, callStmt) {} // TODO
// TEST(FlowParser, postscriptStmt) {} // TODO
