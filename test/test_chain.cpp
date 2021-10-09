#include "utils.h"
#include "gtest/gtest.h"

using namespace std;
using namespace matrixchain;

TEST(Chain, MCP) {
  shared_ptr<Expr> A(new Operand("A1", {30, 35}));
  shared_ptr<Expr> B(new Operand("A2", {35, 15}));
  shared_ptr<Expr> C(new Operand("A3", {15, 5}));
  shared_ptr<Expr> D(new Operand("A4", {5, 10}));
  shared_ptr<Expr> E(new Operand("A5", {10, 20}));
  shared_ptr<Expr> F(new Operand("A6", {20, 25}));
  vector<shared_ptr<Expr>> operands{A, B, C, D, E, F};
  long result = getMCPFlops(operands);
  EXPECT_EQ(result, 15125);
}

TEST(Chain, PropagationRules) {
  shared_ptr<Expr> a(new Operand("A", {20, 20}));
  a->setProperties({Expr::ExprProperty::LOWER_TRIANGULAR});
  shared_ptr<Expr> b(new Operand("B", {20, 20}));
  b->setProperties({Expr::ExprProperty::UPPER_TRIANGULAR});

  auto e = mul(a, trans(b));
  EXPECT_EQ(e->isLowerTriangular(), true);
}
