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

#include "chain.h"
#include "llvm/Support/Casting.h"
#include <algorithm>

template <Expr::ExprProperty P> bool isX(Operand *operand) {
  auto inferredProperties = operand->getProperties();
  return std::any_of(inferredProperties.begin(), inferredProperties.end(),
                     [](Expr::ExprProperty p) { return p == P; });
}

bool Operand::isUpperTriangular() {
  return isX<ExprProperty::UPPER_TRIANGULAR>(this);
}

bool Operand::isLowerTriangular() {
  return isX<ExprProperty::LOWER_TRIANGULAR>(this);
}

bool Operand::isSquare() { return isX<ExprProperty::SQUARE>(this); }

bool Operand::isSymmetric() { return isX<ExprProperty::SYMMETRIC>(this); }

bool Operand::isFullRank() { return isX<ExprProperty::FULL_RANK>(this); }

bool Operand::isSPD() { return isX<ExprProperty::SPD>(this); }

// ----------------------------------------------------------------------

bool UnaryOp::isUpperTriangular() {
  auto kind = this->getKind();
  switch (kind) {
  case UnaryOpKind::TRANSPOSE:
    return child->isLowerTriangular();
  default:
    assert(0 && "UNK");
  }
  return false;
}

bool UnaryOp::isLowerTriangular() {
  auto kind = this->getKind();
  switch (kind) {
  case UnaryOpKind::TRANSPOSE:
    return child->isUpperTriangular();
  default:
    assert(0 && "UNK");
  }
  return false;
}

bool UnaryOp::isSquare() {
  auto kind = this->getKind();
  switch (kind) {
  case UnaryOpKind::TRANSPOSE:
    return child->isSquare();
  default:
    assert(0 && "UNK");
  }
  return false;
}

bool UnaryOp::isSymmetric() {
  auto kind = this->getKind();
  switch (kind) {
  case UnaryOpKind::TRANSPOSE:
    return child->isSymmetric() || child->isSPD();
  default:
    assert(0 && "UNK");
  }
  return false;
}

bool UnaryOp::isFullRank() {
  auto kind = this->getKind();
  switch (kind) {
  case UnaryOpKind::TRANSPOSE:
  case UnaryOpKind::INVERSE:
    return child->isFullRank();
  default:
    assert(0 && "UNK");
  }
  return false;
}

bool UnaryOp::isSPD() {
  assert(0 && "no impl");
  return false;
}

// ----------------------------------------------------------------------

bool BinaryOp::isUpperTriangular() {
  auto kind = this->getKind();
  switch (kind) {
  case BinaryOp::BinaryOpKind::MUL:
    return childLeft->isUpperTriangular() && childRight->isUpperTriangular();
  default:
    assert(0 && "UNK");
  }
  return false;
}

bool BinaryOp::isLowerTriangular() {
  auto kind = this->getKind();
  switch (kind) {
  case BinaryOp::BinaryOpKind::MUL:
    return childLeft->isLowerTriangular() && childRight->isLowerTriangular();
  default:
    assert(0 && "UNK");
  }
  return false;
}

bool BinaryOp::isSquare() { assert(0 && "no impl"); }

// TODO: what other cases?
bool BinaryOp::isSymmetric() { return this->isSPD(); }

bool BinaryOp::isFullRank() {
  // assert(0 && "no impl");
  return false;
}

// see:
// https://github.com/HPAC/linnea/blob/c8fb5d1f64666bf63d35859484a5041ff75dbb90/linnea/algebra/property_inference.py#L109
// TODO: miss check left.columns >= left.rows
bool BinaryOp::isSPD() {
  auto kind = this->getKind();
  switch (kind) {
  case BinaryOp::BinaryOpKind::MUL:
    return childLeft->isFullRank() && childLeft->isTransposeOf(childRight);
  default:
    assert(0 && "UNK");
  }
  return false;
}

// ----------------------------------------------------------------------

bool NaryOp::isUpperTriangular() {
  return std::all_of(
      this->getChildren().begin(), this->getChildren().end(),
      [](shared_ptr<Expr> child) { return child->isUpperTriangular(); });
}

bool NaryOp::isLowerTriangular() {
  return std::all_of(
      this->getChildren().begin(), this->getChildren().end(),
      [](shared_ptr<Expr> child) { return child->isLowerTriangular(); });
}
