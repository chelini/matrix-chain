#include "chain.h"
#include "gtest/gtest.h"

using namespace std;
using namespace matrixchain;

TEST(Chain, ownership) {
  Operand *A = new Operand("A1", {1, 1});
  Operand *B = new Operand("A1", {1, 1});
  std::unique_ptr<Expr> G(mul(trans(A), mul(A, mul(trans(A), B))));
  std::unique_ptr<Expr> L(mul(A, B));
  walk(L.get());

  delete A;
  delete B;
}

TEST(Chain, MCP) {
  Operand *A = new Operand("A1", {30, 35});
  Operand *B = new Operand("A2", {35, 15});
  Operand *C = new Operand("A3", {15, 5});
  // Operand *D = new Operand("A4", {5, 10});
  // Operand *E = new Operand("A5", {10, 20});
  // Operand *F = new Operand("A6", {20, 25});
  // std::unique_ptr<Expr> G(mul(A, mul(B, mul(C, mul(D, mul(E, F))))));
  std::unique_ptr<Expr> G(mul(A, mul(B, C)));

  long result = getMCPFlops(G.get());
  EXPECT_EQ(result, 30250);

  delete A;
  delete B;
  delete C;
  // delete D;
  // delete E;
  // delete F;
}
/*
TEST(Chain, MCPVariadicMul) {
  Operand *A = new Operand("A1", {30, 35});
  Operand *B = new Operand("A2", {35, 15});
  Operand *C = new Operand("A3", {15, 5});
  Operand *D = new Operand("A4", {5, 10});
  Operand *E = new Operand("A5", {10, 20});
  Operand *F = new Operand("A6", {20, 25});
  std::unique_ptr<Expr>G(mul(A, B, C, D, E, F));
  long result = getMCPFlops(G.get());
  EXPECT_EQ(result, 30250);
  delete A;
  delete B;
  delete C;
  delete D;
  delete E;
  delete F;
}

// Expect cost to be n^2 * m * 2 -> 20 * 20 * 15 * 2
TEST(Chain, Cost) {
  Operand *A = new Operand("A", {20, 20});
  Operand *B = new Operand("B", {20, 15});
  std::unique_ptr<Expr>E(mul(A, B));
  long result = getMCPFlops(E.get());
  EXPECT_EQ(result, (20 * 20 * 15) << 1);
  delete A;
  delete B;
}

// Expect cost to be n^2 * m as A is lower triangular
// lower triangular are square, verify?
TEST(Chain, CostWithProperty) {
  Operand *A = new Operand("A", {20, 20});
  Operand *B = new Operand("B", {20, 15});
  A->setProperties({Expr::ExprProperty::LOWER_TRIANGULAR});
  std::unique_ptr<Expr>M(mul(A, B));
  long result = getMCPFlops(M.get());
  EXPECT_EQ(result, (20 * 20 * 15));
  delete A;
  delete B;
}

// The product of two upper (lower) triangular matrices is upper (lower)
// triangular matrix.
TEST(Chain, PropagationRulesUpperTimesUpper) {
  Operand *A = new Operand("A", {20, 20});
  A->setProperties({Expr::ExprProperty::UPPER_TRIANGULAR});
  Operand *B = new Operand("B", {20, 20});
  B->setProperties({Expr::ExprProperty::UPPER_TRIANGULAR});
  std::unique_ptr<Expr>M(mul(A, B));
  EXPECT_EQ(M->isUpperTriangular(), true);
  delete A;
  delete B;
}

TEST(Chain, PropagationRulesLowerTimesLower) {
  Operand *A = new Operand("A", {20, 20});
  A->setProperties({Expr::ExprProperty::LOWER_TRIANGULAR});
  Operand *B = new Operand("B", {20, 20});
  B->setProperties({Expr::ExprProperty::LOWER_TRIANGULAR});
  std::unique_ptr<Expr>aTimesB(mul(A, B));
  EXPECT_EQ(aTimesB->isLowerTriangular(), true);
  std::unique_ptr<Expr>aTimesBTransTrans(mul(A, trans(trans(B))));
  EXPECT_EQ(aTimesBTransTrans->isLowerTriangular(), true);
  std::unique_ptr<Expr>aTransTransTimesB(mul(trans(trans(A)), B));
  EXPECT_EQ(aTransTransTimesB->isLowerTriangular(), true);
  delete A;
  delete B;
}

// If you transpose an upper (lower) triangular matrix, you get a lower (upper)
// triangular matrix.
TEST(Chain, PropagationRulesTransposeUpper) {
  Operand *A = new Operand("A", {20, 20});
  A->setProperties({Expr::ExprProperty::UPPER_TRIANGULAR});
  std::unique_ptr<Expr>T(trans(A));
  EXPECT_EQ(T->isLowerTriangular(), true);
  delete A;
}

TEST(Chain, PropagationRulesTransposeLower) {
  shared_ptr<Expr> A(new Operand("A", {20, 20}));
  A->setProperties({Expr::ExprProperty::LOWER_TRIANGULAR});
  auto T = trans(A);
  EXPECT_EQ(T->isUpperTriangular(), true);
}

TEST(Chain, PropagationRulesTransposeMultipleTimes) {
  shared_ptr<Expr> A(new Operand("A", {20, 20}));
  A->setProperties({Expr::ExprProperty::UPPER_TRIANGULAR});
  auto T = trans(trans(A));
  EXPECT_EQ(T->isUpperTriangular(), true);
  T = trans(trans(trans(A)));
  EXPECT_EQ(T->isLowerTriangular(), true);
}

TEST(Chain, PropagationRulesIsFullRank) {
  shared_ptr<Expr> A(new Operand("A", {20, 20}));
  A->setProperties({Expr::ExprProperty::FULL_RANK});
  auto T = trans(A);
  EXPECT_EQ(T->isFullRank(), true);
  auto I = inv(A);
  EXPECT_EQ(I->isFullRank(), true);
  auto IT = inv(trans(A));
  EXPECT_EQ(IT->isFullRank(), true);
}

TEST(Chain, PropagationRulesIsSPD) {
  shared_ptr<Expr> A(new Operand("A", {20, 20}));
  A->setProperties({Expr::ExprProperty::FULL_RANK});
  auto SPD = mul(trans(A), A);
  EXPECT_EQ(SPD->isSPD(), true);
}

TEST(Chain, kernelCostWhenSPD) {
  shared_ptr<Expr> A(new Operand("A", {20, 20}));
  shared_ptr<Expr> B(new Operand("B", {20, 15}));
  A->setProperties({Expr::ExprProperty::FULL_RANK});
  long cost = 0;
  auto E = mul(mul(trans(A), A), B);
  getKernelCostTopLevelExpr(E, cost);
  EXPECT_EQ(cost, 6000);
  cost = 0;
  getKernelCostFullExpr(E, cost);
  EXPECT_EQ(cost, 22000);
}

TEST(Chain, CountFlopsIsSPD) {
  shared_ptr<Expr> A(new Operand("A", {20, 20}));
  shared_ptr<Expr> B(new Operand("B", {20, 15}));
  A->setProperties({Expr::ExprProperty::FULL_RANK});
  auto E = mul(mul(trans(A), A), B);
  auto result = getMCPFlops(E);
  EXPECT_EQ(result, 22000);
}

TEST(Chain, CountFlopsIsSymmetric) {
  shared_ptr<Expr> A(new Operand("A", {20, 20}));
  shared_ptr<Expr> B(new Operand("B", {20, 15}));
  auto E = mul(mul(trans(A), A), B);
  auto result = getMCPFlops(E);
  EXPECT_EQ(result, 22000);
  auto F = mul(mul(A, trans(A)), B);
  result = getMCPFlops(F);
  EXPECT_EQ(result, 22000);
  auto G = mul(A, trans(A), B);
  result = getMCPFlops(G);
  EXPECT_EQ(result, 22000);
}
*/
