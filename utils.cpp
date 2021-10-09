#include "utils.h"
#include "llvm/Support/Casting.h"
#include <iostream>
#include <limits>

using namespace matrixchain;

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

bool UnaryOp::isUpperTriangular() {
  auto kind = this->getKind();
  switch (kind) {
  case UnaryOp::UnaryOpKind::TRANSPOSE:
    return child->isLowerTriangular();
  default:
    assert(0 && "UNK");
  }
  return false;
}

bool UnaryOp::isLowerTriangular() {
  auto kind = this->getKind();
  switch (kind) {
  case UnaryOp::UnaryOpKind::TRANSPOSE:
    return child->isUpperTriangular();
  default:
    assert(0 && "UNK");
  }
  return false;
}

bool Operand::isUpperTriangular() {
  // TODO: make this better
  bool found = false;
  for (auto property : properties) {
    if (property == Expr::ExprProperty::UPPER_TRIANGULAR)
      found = true;
  }
  return found;
}

bool Operand::isLowerTriangular() {
  // TODO: make this better
  bool found = false;
  for (auto property : properties) {
    if (property == Expr::ExprProperty::LOWER_TRIANGULAR)
      found = true;
  }
  return found;
}

/// infer properties for UnaryExpr.
vector<Expr::ExprProperty> UnaryOp::inferProperty() {
  UnaryOpKind kind = this->getKind();
  switch (kind) {
  case UnaryOpKind::TRANSPOSE: {
    if (this->isUpperTriangular())
      return {Expr::ExprProperty::UPPER_TRIANGULAR};
    return {};
  }
  default:
    assert(0 && "not handled");
  }
  assert(0 && "unreachable");
  return {};
}

/// print an array of expression properties.
static void printProperties(vector<Expr::ExprProperty> properties) {
  for (size_t i = 0, e = properties.size(); i < e; i++) {
    switch (properties[i]) {
    case Expr::ExprProperty::LOWER_TRIANGULAR:
      cout << "LOWER_TRIANGULAR";
      break;
    case Expr::ExprProperty::UPPER_TRIANGULAR:
      cout << "UPPER_TRIANGULAR";
      break;
    default:
      assert(0 && "UNK");
    }
    if (i != e - 1)
      cout << ", ";
  }
}

/// print the shape of the operand.
static void printShape(vector<int> shape) {
  for (size_t i = 0, e = shape.size(); i < e; i++) {
    cout << shape[i];
    if (i != e - 1)
      cout << ", ";
  }
}

#define LEVEL_SPACES 2

/// Walk a generic expression.
void walk(shared_ptr<Expr> node, int level) {
  if (node) {
    if (auto binaryOp = llvm::dyn_cast_or_null<BinaryOp>(node.get())) {
      switch (binaryOp->getKind()) {
      case BinaryOp::BinaryOpKind::MUL:
        cout << string(level, ' ') << "(*\n";
        break;
      default:
        cout << "UNK";
      }
      walk(binaryOp->getLeftChild(), level + LEVEL_SPACES);
      cout << " \n";
      walk(binaryOp->getRightChild(), level + LEVEL_SPACES);
    } // binaryOp
    if (auto unaryOp = llvm::dyn_cast_or_null<UnaryOp>(node.get())) {
      switch (unaryOp->getKind()) {
      case UnaryOp::UnaryOpKind::TRANSPOSE:
        cout << string(level, ' ') << "transpose(";
        break;
      case UnaryOp::UnaryOpKind::INVERSE:
        cout << string(level, ' ') << "inverse(";
        break;
      default:
        cout << "UNK";
      }
      walk(unaryOp->getChild());
      cout << ")";
    } // unaryOp
    if (auto operand = llvm::dyn_cast_or_null<Operand>(node.get())) {
      cout << string(level, ' ') << operand->getName() << " [";
      printProperties(operand->getProperties());
      cout << "] [";
      printShape(operand->getShape());
      cout << "]";
    } // operand
  }
}

/// Multiply two expressions.
shared_ptr<Expr> mul(shared_ptr<Expr> left, shared_ptr<Expr> right) {
  assert(left && "left expr must be non null");
  assert(right && "right expr must be non null");
  return shared_ptr<Expr>(
      new BinaryOp(left, right, BinaryOp::BinaryOpKind::MUL));
}

/// invert an expression.
shared_ptr<Expr> inv(shared_ptr<Expr> child) {
  assert(child && "child expr must be non null");
  return shared_ptr<Expr>(new UnaryOp(child, UnaryOp::UnaryOpKind::INVERSE));
}

/// transpose an expression.
shared_ptr<Expr> trans(shared_ptr<Expr> child) {
  assert(child && "child expr must be non null");
  return shared_ptr<Expr>(new UnaryOp(child, UnaryOp::UnaryOpKind::TRANSPOSE));
}

static vector<long> getPVector(vector<shared_ptr<Expr>> exprs) {
  vector<long> pVector;
  for (auto expr : exprs) {
    auto operand = llvm::dyn_cast_or_null<Operand>(expr.get());
    assert(operand && "must be non null");
    auto shape = operand->getShape();
    if (!pVector.size()) {
      pVector.push_back(shape[0]);
      pVector.push_back(shape[1]);
    } else {
      pVector.push_back(shape[1]);
    }
  }
  return pVector;
}

static void printOptimalParens(const vector<vector<long>> &s, size_t i,
                               size_t j, vector<shared_ptr<Expr>> operands) {
  if (i == j) {
    cout << " ";
    auto operand = llvm::dyn_cast_or_null<Operand>(operands[i - 1].get());
    assert(operand && "must be non null");
    cout << operand->getName();
    cout << "  ";
  } else {
    cout << "(";
    printOptimalParens(s, i, s[i][j], operands);
    printOptimalParens(s, s[i][j] + 1, j, operands);
    cout << ")";
  }
}

static void collectOperandsImpl(shared_ptr<Expr> node,
                                vector<shared_ptr<Expr>> &operands) {
  if (node) {
    if (auto binaryOp = llvm::dyn_cast_or_null<BinaryOp>(node.get())) {
      collectOperandsImpl(binaryOp->getLeftChild(), operands);
      collectOperandsImpl(binaryOp->getRightChild(), operands);
    }
    if (auto unaryOp = llvm::dyn_cast_or_null<UnaryOp>(node.get())) {
      collectOperandsImpl(unaryOp->getChild(), operands);
    }
    if (auto operand = llvm::dyn_cast_or_null<Operand>(node.get())) {
      operands.push_back(node);
    }
  }
}

static vector<shared_ptr<Expr>> collectOperands(shared_ptr<Expr> &expr) {
  vector<shared_ptr<Expr>> operands;
  collectOperandsImpl(expr, operands);
  return operands;
}

#if DEBUG
static void print(vector<vector<shared_ptr<Expr>>> &tmps,
                  bool bitLayout = false) {
  int rows = tmps.size();
  int cols = tmps[0].size();

  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      if (tmps[i][j]) {
        if (bitLayout)
          cout << "1 ";
        else
          walk(tmps[i][j]);
      } else {
        if (bitLayout)
          cout << "0 ";
      }
    }
    cout << "\n";
  }
}
#endif

struct ResultMCP {
  vector<vector<long>> m;
  vector<vector<long>> s;
};

ResultMCP runMCP(shared_ptr<Expr> &expr) {
  vector<shared_ptr<Expr>> operands = collectOperands(expr);
  vector<long> pVector = getPVector(operands);
  const size_t n = pVector.size();
  vector<vector<long>> m(n, vector<long>(n, std::numeric_limits<long>::max()));
  vector<vector<long>> s(n, vector<long>(n, std::numeric_limits<long>::max()));

  // store symbolic temporary variables representing sub-chains.
  vector<vector<shared_ptr<Expr>>> tmps(n,
                                        vector<shared_ptr<Expr>>(n, nullptr));

  for (size_t i = 0; i < n - 1; i++)
    tmps[i + 1][i + 1] = operands.at(i);

  for (size_t i = 0; i < n; i++)
    m[i][i] = 0;

  size_t j = 0;
  long q = 0;
  for (size_t l = 2; l < n; l++) {
    for (size_t i = 1; i < n - l + 1; i++) {
      j = i + l - 1;
      m[i][j] = std::numeric_limits<long>::max();
      for (size_t k = i; k <= j - 1; k++) {
        long cost = pVector.at(i - 1) * pVector.at(k) * pVector.at(j);
        q = m[i][k] + m[k + 1][j] + cost;
        if (q < m[i][j]) {
          tmps[i][j] = mul(tmps[i][k], tmps[k + 1][j]);
          tmps[i][j].get()->inferProperty();
          m[i][j] = q;
          s[i][j] = k;
        }
      }
    }
  }

#if DEBUG
  cout << "\n\n----tmps----\n";
  print(tmps, true);
  cout << "\n";
  walk(tmps[1][tmps.size() - 1]);

  cout << "\n\n-----s------\n";
  int rows = s.size();
  int cols = s[0].size();
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      if (s[i][j] == std::numeric_limits<long>::max())
        cout << "- ";
      else
        cout << s[i][j] << " ";
    }
    cout << "\n";
  }
  cout << "\n-----m------\n";
  rows = m.size();
  cols = m[0].size();
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      if (m[i][j] == std::numeric_limits<long>::max())
        cout << "- ";
      else
        cout << m[i][j] << " ";
    }
    cout << "\n";
  }
  cout << "\n";
  printOptimalParens(s, 1, operands.size(), operands);
  cout << "\n\n";
#endif
  return {m, s};
}

long getMCPFlops(shared_ptr<Expr> &expr) {
  ResultMCP result = runMCP(expr);
  auto m = result.m;
  return m[1][m.size() - 1];
}
