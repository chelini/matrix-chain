#include "llvm/Support/Casting.h"
#include <cassert>
#include <iostream>
#include <limits>
#include <memory>
#include <vector>

using namespace std;

#define LEVEL_SPACES 2

class BinaryOp;
class UnaryOp;
class Operand;

/// Generic expr of type BINARY, UNARY or OPERAND.
class Expr {
public:
  enum class ExprKind { BINARY, UNARY, OPERAND };
  enum class ExprProperty { UPPER_TRIANGULAR, LOWER_TRIANGULAR };

private:
  const ExprKind kind;

public:
  ExprKind getKind() const { return kind; }
  virtual vector<Expr::ExprProperty> inferProperty() = 0;
  virtual void setProperties(vector<Expr::ExprProperty> properties) {
    assert(0 && "can set properties only for operands");
  };
  virtual bool isUpperTriangular() = 0;

protected:
  Expr() = delete;
  Expr(ExprKind kind) : kind(kind){};
};

/// Binary operation (i.e., MUL).
class BinaryOp : public Expr {
public:
  enum class BinaryOpKind { MUL };

private:
  shared_ptr<Expr> childLeft;
  shared_ptr<Expr> childRight;
  BinaryOpKind kind;

public:
  BinaryOp() = delete;
  BinaryOp(shared_ptr<Expr> left, shared_ptr<Expr> right, BinaryOpKind kind)
      : Expr(ExprKind::BINARY), childLeft(left), childRight(right),
        kind(kind){};
  BinaryOpKind getKind() { return kind; };
  shared_ptr<Expr> getLeftChild() { return childLeft; };
  shared_ptr<Expr> getRightChild() { return childRight; };
  vector<Expr::ExprProperty> inferProperty() { return {}; };
  bool isUpperTriangular() { return false; };
  static bool classof(const Expr *expr) {
    return expr->getKind() == ExprKind::BINARY;
  };
};

/// Unary operation like transpose or inverse.
class UnaryOp : public Expr {
public:
  enum class UnaryOpKind { TRANSPOSE, INVERSE };

private:
  shared_ptr<Expr> child;
  UnaryOpKind kind;

public:
  UnaryOp() = delete;
  UnaryOp(shared_ptr<Expr> child, UnaryOpKind kind)
      : Expr(ExprKind::UNARY), child(child), kind(kind){};
  shared_ptr<Expr> getChild() { return child; };
  UnaryOpKind getKind() { return kind; };
  vector<Expr::ExprProperty> inferProperty();
  bool isUpperTriangular() {
    cout << "unary"
         << "\n";
    return false;
  }
  static bool classof(const Expr *expr) {
    return expr->getKind() == ExprKind::UNARY;
  };
};

/// Generic operand (i.e., matrix or vector).
class Operand : public Expr {
private:
  string name;
  vector<int> shape;
  vector<Expr::ExprProperty> properties;

public:
  Operand() = delete;
  Operand(string name, vector<int> shape)
      : Expr(ExprKind::OPERAND), name(name), shape(shape){};
  string getName() { return name; };
  vector<int> getShape() { return shape; };
  vector<Expr::ExprProperty> getProperties() { return properties; };
  void setProperties(vector<Expr::ExprProperty> properties) {
    this->properties = properties;
  };
  vector<Expr::ExprProperty> inferProperty() { return properties; };
  bool isUpperTriangular() {
    cout << "operand"
         << "\n";
    return false;
  }
  static bool classof(const Expr *expr) {
    return expr->getKind() == ExprKind::OPERAND;
  };
};

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

/// infer properties for UnaryExpr.
vector<Expr::ExprProperty> UnaryOp::inferProperty() {
  UnaryOpKind kind = this->getKind();
  switch (kind) {
  case UnaryOpKind::TRANSPOSE: {
    bool isUpT = this->getChild().get()->isUpperTriangular();
    if (isUpT)
      return {Expr::ExprProperty::UPPER_TRIANGULAR};
    return {};
  }
  default:
    assert(0 && "not handled");
  }
  assert(0 && "unreachable");
  return {};
}

/// Walk a generic expression.
void walk(shared_ptr<Expr> node, int level = 0) {
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
        cout << string(level, ' ') << "t(";
        break;
      case UnaryOp::UnaryOpKind::INVERSE:
        cout << string(level, ' ') << "i(";
        break;
      default:
        cout << "UNK";
      }
      walk(unaryOp->getChild());
      cout << ")";
    } // unaryOp
    if (auto operand = llvm::dyn_cast_or_null<Operand>(node.get())) {
      cout << string(level, ' ') << operand->getName();
    } // operand
  }
}

vector<long> getPVector(vector<Operand> &operands) {
  cout << __func__ << "\n";
  vector<long> pVector;
  for (auto operand : operands) {
    auto shape = operand.getShape();
    if (!pVector.size()) {
      pVector.push_back(shape[0]);
      pVector.push_back(shape[1]);
    } else {
      pVector.push_back(shape[1]);
    }
  }
  return pVector;
}

void printOptimalParens(const vector<vector<long>> &s, size_t i, size_t j,
                        vector<Operand> operands) {
  if (i == j) {
    cout << " ";
    cout << operands[i - 1].getName();
    cout << "  ";
  } else {
    cout << "(";
    printOptimalParens(s, i, s[i][j], operands);
    printOptimalParens(s, s[i][j] + 1, j, operands);
    cout << ")";
  }
}

vector<Operand> collectOperands(vector<shared_ptr<Expr>> &exprs) {
  cout << __func__ << "\n";
  vector<Operand> operands;
  for (auto expr : exprs) {
    if (Operand *operand = llvm::dyn_cast_or_null<Operand>(expr.get()))
      operands.push_back(*operand);
    else if (auto unary = llvm::dyn_cast_or_null<UnaryOp>(expr.get())) {
      if (Operand *operand =
              llvm::dyn_cast_or_null<Operand>(unary->getChild().get()))
        operands.push_back(*operand);
    } else {
      assert(0 && "only operand or unary op here");
    }
  }
  assert(exprs.size() == operands.size() && "lost something");
  return operands;
}

void print(vector<vector<shared_ptr<Expr>>> &tmps, bool bitLayout = false) {
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

vector<vector<long>> getOptimalSplit(vector<shared_ptr<Expr>> &exprs) {
  cout << __func__ << "\n";

  vector<Operand> operands = collectOperands(exprs);
  vector<long> pVector = getPVector(operands);
  const size_t n = pVector.size();
  vector<vector<long>> m(n, vector<long>(n, std::numeric_limits<long>::max()));
  vector<vector<long>> s(n, vector<long>(n, std::numeric_limits<long>::max()));

  // store symbolic temporary variables representing sub-chains.
  vector<vector<shared_ptr<Expr>>> tmps(n,
                                        vector<shared_ptr<Expr>>(n, nullptr));

  for (size_t i = 0; i < n - 1; i++)
    tmps[i + 1][i + 1] = exprs.at(i);

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

  return s;
}

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

  cout << "\n\n";
  return 0;
}
