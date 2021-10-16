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
    return child->isSymmetric();
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

bool BinaryOp::isSquare() {
  auto kind = this->getKind();
  switch (kind) {
  case BinaryOp::BinaryOpKind::MUL:
    return childLeft->isSquare() && childRight->isSquare();
  default:
    assert(0 && "UNK");
  }
  return false;
}

bool BinaryOp::isSymmetric() {
  auto kind = this->getKind();
  switch (kind) {
  case BinaryOp::BinaryOpKind::MUL:
    return childLeft->isSymmetric() && childRight->isSymmetric();
  default:
    assert(0 && "UNK");
  }
  return false;
}

bool BinaryOp::isFullRank() {
  assert(0 && "no impl");
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
