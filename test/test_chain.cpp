/*
Copyright 2021 Lorenzo Chelini <l.chelini@icloud.com> or
<lorenzo.chelini@huawei.c om>

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "chain.h"
#include "gtest/gtest.h"

using namespace std;
using namespace matrixchain;

TEST(Chain, MCP) {
  details::ScopedContext ctx;
  auto *A = new Operand("A1", {30, 35});
  auto *B = new Operand("A2", {35, 15});
  auto *C = new Operand("A3", {15, 5});
  auto *D = new Operand("A4", {5, 10});
  auto *E = new Operand("A5", {10, 20});
  auto *F = new Operand("A6", {20, 25});
  auto G = mul(A, mul(B, mul(C, mul(D, mul(E, F)))));
  long result = getMCPFlops(G);
  EXPECT_EQ(result, 30250);
}

TEST(Chain, MCPVariadicMul) {
  details::ScopedContext ctx;
  auto *A = new Operand("A1", {30, 35});
  auto *B = new Operand("A2", {35, 15});
  auto *C = new Operand("A3", {15, 5});
  auto *D = new Operand("A4", {5, 10});
  auto *E = new Operand("A5", {10, 20});
  auto *F = new Operand("A6", {20, 25});
  auto G = mul(A, B, C, D, E, F);
  long result = getMCPFlops(G);
  EXPECT_EQ(result, 30250);
}

// Expect cost to be n^2 * m * 2 -> 20 * 20 * 15 * 2
TEST(Chain, Cost) {
  details::ScopedContext ctx;
  auto *A = new Operand("A", {20, 20});
  auto *B = new Operand("B", {20, 15});
  auto *E = mul(A, B);
  long result = getMCPFlops(E);
  EXPECT_EQ(result, (20 * 20 * 15) << 1);
}

// Expect cost to be n^2 * m as A is lower triangular
// lower triangular are square, verify?
TEST(Chain, CostWithProperty) {
  ScopedContext ctx;
  auto *A = new Operand("A", {20, 20});
  auto *B = new Operand("B", {20, 15});
  A->setProperties({Expr::ExprProperty::LOWER_TRIANGULAR});
  auto *M = mul(A, B);
  long result = getMCPFlops(M);
  EXPECT_EQ(result, (20 * 20 * 15));
}

// The product of two upper (lower) triangular matrices is upper (lower)
// triangular matrix.
TEST(Chain, PropagationRulesUpperTimesUpper) {
  ScopedContext ctx;
  auto *A = new Operand("A", {20, 20});
  A->setProperties({Expr::ExprProperty::UPPER_TRIANGULAR});
  auto *B = new Operand("B", {20, 20});
  B->setProperties({Expr::ExprProperty::UPPER_TRIANGULAR});
  auto *M = mul(A, B);
  EXPECT_EQ(M->isUpperTriangular(), true);
}

TEST(Chain, PropagationRulesLowerTimesLower) {
  ScopedContext ctx;
  auto *A = new Operand("A", {20, 20});
  A->setProperties({Expr::ExprProperty::LOWER_TRIANGULAR});
  auto *B = new Operand("B", {20, 20});
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
  ScopedContext ctx;
  auto *A = new Operand("A", {20, 20});
  A->setProperties({Expr::ExprProperty::UPPER_TRIANGULAR});
  auto T = trans(A);
  EXPECT_EQ(T->isLowerTriangular(), true);
}

TEST(Chain, PropagationRulesTransposeLower) {
  ScopedContext ctx;
  auto *A = new Operand("A", {20, 20});
  A->setProperties({Expr::ExprProperty::LOWER_TRIANGULAR});
  auto T = trans(A);
  EXPECT_EQ(T->isUpperTriangular(), true);
}

TEST(Chain, PropagationRulesTransposeMultipleTimes) {
  ScopedContext ctx;
  auto *A = new Operand("A", {20, 20});
  A->setProperties({Expr::ExprProperty::UPPER_TRIANGULAR});
  auto T = trans(trans(A));
  EXPECT_EQ(T->isUpperTriangular(), true);
  T = trans(trans(trans(A)));
  EXPECT_EQ(T->isLowerTriangular(), true);
}

TEST(Chain, PropagationRulesIsFullRank) {
  ScopedContext ctx;
  auto *A = new Operand("A", {20, 20});
  A->setProperties({Expr::ExprProperty::FULL_RANK});
  auto T = trans(A);
  EXPECT_EQ(T->isFullRank(), true);
  auto I = inv(A);
  EXPECT_EQ(I->isFullRank(), true);
  auto IT = inv(trans(A));
  EXPECT_EQ(IT->isFullRank(), true);
}

TEST(Chain, PropagationRulesIsSPD) {
  ScopedContext ctx;
  auto *A = new Operand("A", {20, 20});
  A->setProperties({Expr::ExprProperty::FULL_RANK});
  auto SPD = mul(trans(A), A);
  EXPECT_EQ(SPD->isSPD(), true);
}

TEST(Chain, kernelCostWhenSPD) {
  ScopedContext ctx;
  auto *A = new Operand("A", {20, 20});
  auto *B = new Operand("B", {20, 15});
  A->setProperties({Expr::ExprProperty::FULL_RANK});
  long cost = 0;
  auto E = mul(mul(trans(A), A), B);
  cost = getMCPFlops(E);
  EXPECT_EQ(cost, 22000);
}

TEST(Chain, CountFlopsIsSPD) {
  ScopedContext ctx;
  auto *A = new Operand("A", {20, 20});
  auto *B = new Operand("B", {20, 15});
  A->setProperties({Expr::ExprProperty::FULL_RANK});
  auto E = mul(mul(trans(A), A), B);
  auto result = getMCPFlops(E);
  EXPECT_EQ(result, 22000);
}

TEST(Chain, CountFlopsIsSymmetric) {
  ScopedContext ctx;
  auto *A = new Operand("A", {20, 20});
  auto *B = new Operand("B", {20, 15});
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

TEST(Chain, areSameTree) {
  ScopedContext ctx;
  auto *A = new Operand("A", {20, 20});
  auto *B = new Operand("B", {20, 20});
  auto *C = new Operand("C", {20, 20});
  auto *exp1 = trans(mul(A, B));
  auto *exp2 = trans(mul(A, B));
  auto *exp3 = trans(mul(A, C));
  bool is = exp1->isSame(exp2);
  EXPECT_EQ(is, true);
  is = exp1->isSame(exp3);
  EXPECT_EQ(is, false);
}

TEST(Chain, NormalForm) {
  ScopedContext ctx;
  auto *A = new Operand("A", {20, 20});
  auto *B = new Operand("B", {20, 20});
  auto *C = new Operand("C", {20, 20});
  auto *expr = trans(mul(A, mul(B, C)));
  walk(expr);
  cout << "\n\n normal form --->\n\n";
  auto *normalForm = expr->getNormalForm();
  walk(normalForm);
}
