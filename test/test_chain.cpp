#include "chain.h"
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
  auto G = mul(A, mul(B, mul(C, mul(D, mul(E, F)))));
  long result = getMCPFlops(G);
  EXPECT_EQ(result, 30250);
}

TEST(Chain, MCPVariadicMul) {
  shared_ptr<Expr> A(new Operand("A1", {30, 35}));
  shared_ptr<Expr> B(new Operand("A2", {35, 15}));
  shared_ptr<Expr> C(new Operand("A3", {15, 5}));
  shared_ptr<Expr> D(new Operand("A4", {5, 10}));
  shared_ptr<Expr> E(new Operand("A5", {10, 20}));
  shared_ptr<Expr> F(new Operand("A6", {20, 25}));
  auto G = mul(A, B, C, D, E, F);
  long result = getMCPFlops(G);
  EXPECT_EQ(result, 30250);
}

// Expect cost to be n^2 * m * 2 -> 20 * 20 * 15 * 2
TEST(Chain, Cost) {
  shared_ptr<Expr> A(new Operand("A", {20, 20}));
  shared_ptr<Expr> B(new Operand("B", {20, 15}));
  auto E = mul(A, B);
  long result = getMCPFlops(E);
  EXPECT_EQ(result, (20 * 20 * 15) << 1);
}

// Expect cost to be n^2 * m as A is lower triangular
// lower triangular are square, verify?
TEST(Chain, CostWithProperty) {
  shared_ptr<Expr> A(new Operand("A", {20, 20}));
  shared_ptr<Expr> B(new Operand("B", {20, 15}));
  A->setProperties({Expr::ExprProperty::LOWER_TRIANGULAR});
  auto M = mul(A, B);
  long result = getMCPFlops(M);
  EXPECT_EQ(result, (20 * 20 * 15));
}

// The product of two upper (lower) triangular matrices is upper (lower)
// triangular matrix.
TEST(Chain, PropagationRulesUpperTimesUpper) {
  shared_ptr<Expr> A(new Operand("A", {20, 20}));
  A->setProperties({Expr::ExprProperty::UPPER_TRIANGULAR});
  shared_ptr<Expr> B(new Operand("B", {20, 20}));
  B->setProperties({Expr::ExprProperty::UPPER_TRIANGULAR});
  auto M = mul(A, B);
  EXPECT_EQ(M->isUpperTriangular(), true);
}

TEST(Chain, PropagationRulesLowerTimesLower) {
  shared_ptr<Expr> A(new Operand("A", {20, 20}));
  A->setProperties({Expr::ExprProperty::LOWER_TRIANGULAR});
  shared_ptr<Expr> B(new Operand("B", {20, 20}));
  B->setProperties({Expr::ExprProperty::LOWER_TRIANGULAR});
  auto aTimesB = mul(A, B);
  EXPECT_EQ(aTimesB->isLowerTriangular(), true);
  auto aTimesBTransTrans = mul(A, trans(trans(B)));
  EXPECT_EQ(aTimesBTransTrans->isLowerTriangular(), true);
  auto aTransTransTimesB = mul(trans(trans(A)), B);
  EXPECT_EQ(aTransTransTimesB->isLowerTriangular(), true);
}

// If you transpose an upper (lower) triangular matrix, you get a lower (upper)
// triangular matrix.
TEST(Chain, PropagationRulesTransposeUpper) {
  shared_ptr<Expr> A(new Operand("A", {20, 20}));
  A->setProperties({Expr::ExprProperty::UPPER_TRIANGULAR});
  auto T = trans(A);
  EXPECT_EQ(T->isLowerTriangular(), true);
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
