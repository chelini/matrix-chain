/*
Copyright 2021 Lorenzo Chelini <l.chelini@icloud.com>

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

#include "utils.h"
#include <iostream>

using namespace std;
using namespace matrixchain;

// TODO: RTTI: We shoud avoid getting the raw pointer
// from a shared_ptr<Expr>
int main() {
  cout << __func__ << "\n";
  shared_ptr<Expr> A(new Operand("A1", {30, 35}));
  shared_ptr<Expr> B(new Operand("A2", {35, 15}));
  shared_ptr<Expr> C(new Operand("A3", {15, 5}));
  shared_ptr<Expr> D(new Operand("A4", {5, 10}));
  shared_ptr<Expr> E(new Operand("A5", {10, 20}));
  shared_ptr<Expr> F(new Operand("A6", {20, 25}));
  vector<shared_ptr<Expr>> operands{A, B, C, D, E, F};
  auto result = getOptimalSplit(operands);

  shared_ptr<Expr> a(new Operand("A", {20, 20}));
  a->setProperties({Expr::ExprProperty::LOWER_TRIANGULAR});
  shared_ptr<Expr> b(new Operand("B", {20, 20}));
  b->setProperties({Expr::ExprProperty::UPPER_TRIANGULAR});
  auto e = mul(a, trans(b));
  cout << "lt: " << e->isLowerTriangular() << "\n";

  cout << "\n\n";
  return 0;
}
