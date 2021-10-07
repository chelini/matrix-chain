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

class Expr {
public:
  enum class ExprKind { BINARY, UNARY, OPERAND };

private:
  const ExprKind kind;

public:
  ExprKind getKind() const { return kind; }

protected:
  Expr() = delete;
  Expr(ExprKind kind) : kind(kind){};
};

class BinaryOp : public Expr {
public:
  enum class BinaryOpKind { MUL };

public:
  shared_ptr<Expr> childLeft;
  shared_ptr<Expr> childRight;
  BinaryOpKind kind;

public:
  BinaryOp() = delete;
  BinaryOp(shared_ptr<Expr> left, shared_ptr<Expr> right, BinaryOpKind kind)
      : childLeft(left), childRight(right), kind(kind),
        Expr(ExprKind::BINARY){};
  // BinaryOpKind getKind() { return kind; };
  static bool classof(const Expr *expr) {
    return expr->getKind() == ExprKind::BINARY;
  }
};

class UnaryOp : public Expr {
public:
  enum class UnaryOpKind { TRANS, INV };

public:
  shared_ptr<Expr> child;
  UnaryOpKind kind;

public:
  UnaryOp() = delete;
  UnaryOp(shared_ptr<Expr> child, UnaryOpKind kind)
      : child(child), kind(kind), Expr(ExprKind::UNARY){};
  static bool classof(const Expr *expr) {
    return expr->getKind() == ExprKind::UNARY;
  }
};

class Operand : public Expr {
public:
  enum class OperandProperty { SYMM };

private:
  string name;
  vector<int> shape;
  vector<OperandProperty> properties;

public:
  Operand() = delete;
  Operand(string name, vector<int> shape)
      : name(name), shape(shape), Expr(ExprKind::OPERAND){};
  static bool classof(const Expr *expr) {
    return expr->getKind() == ExprKind::OPERAND;
  }
  string getName() { return name; };
  vector<int> getShape() { return shape; };
  vector<OperandProperty> getProperties() { return properties; };
  void setProperties(vector<OperandProperty> properties) {
    this->properties = properties;
  };
};

void walk(shared_ptr<Expr> node, int level = 0) {
  if (node) {
    if (auto *binaryOp = llvm::dyn_cast_or_null<BinaryOp>(node.get())) {
      switch (binaryOp->kind) {
      case BinaryOp::BinaryOpKind::MUL:
        cout << string(level, ' ') << "(*\n";
        break;
      default:
        cout << "UNK";
      }
      walk(binaryOp->childLeft, level + LEVEL_SPACES);
      cout << " \n";
      walk(binaryOp->childRight, level + LEVEL_SPACES);
    } // binaryOp
    if (auto *unaryOp = llvm::dyn_cast_or_null<UnaryOp>(node.get())) {
      switch (unaryOp->kind) {
      case UnaryOp::UnaryOpKind::TRANS:
        cout << string(level, ' ') << "t(";
        break;
      case UnaryOp::UnaryOpKind::INV:
        cout << string(level, ' ') << "i(";
        break;
      default:
        cout << "UNK";
      }
      walk(unaryOp->child);
      cout << ")";
    } // unaryOp
    if (auto *operand = llvm::dyn_cast_or_null<Operand>(node.get())) {
      cout << string(level, ' ') << operand->getName();
    } // operand
  }
}

vector<long> getPVector(vector<Operand> &operands) {
  vector<long> pVector;
  for (auto matrix : operands) {
    auto shape = matrix.getShape();
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

vector<vector<long>> getOptimalSplit(vector<Operand> &operands) {

  vector<long> pVector = getPVector(operands);
  const size_t n = pVector.size();
  vector<vector<long>> m(n, vector<long>(n, std::numeric_limits<long>::max()));
  vector<vector<long>> s(n, vector<long>(n, std::numeric_limits<long>::max()));

  // store symbolic temporary variables representing sub-chains.
  vector<vector<shared_ptr<Expr>>> tmps(n,
                                        vector<shared_ptr<Expr>>(n, nullptr));

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
        cout << "cost for sub: " << i << " " << j << " --- " << k + 1 << " "
             << j << "\n";
        cout << "is : " << m[i][k] << " " << m[k + 1][j] << "  " << cost
             << "\n";
        q = m[i][k] + m[k + 1][j] + cost;
        if (q < m[i][j]) {
          m[i][j] = q;
          s[i][j] = k;
        }
      }
    }
  }

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
  cout << "-----m------\n";
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
  printOptimalParens(s, 1, operands.size(), operands);
  cout << "\n\n";

  return s;
}

shared_ptr<Expr> mul(shared_ptr<Expr> left, shared_ptr<Expr> right) {
  return shared_ptr<Expr>(
      new BinaryOp(left, right, BinaryOp::BinaryOpKind::MUL));
}

shared_ptr<Expr> inv(shared_ptr<Expr> child) {
  return shared_ptr<Expr>(new UnaryOp(child, UnaryOp::UnaryOpKind::INV));
}

shared_ptr<Expr> trans(shared_ptr<Expr> child) {
  return shared_ptr<Expr>(new UnaryOp(child, UnaryOp::UnaryOpKind::TRANS));
}

int main() {

  shared_ptr<Expr> A(new Operand("A1", {30, 35}));
  Matrix B = Matrix("A2", {35, 15});
  Matrix C = Matrix("A3", {15, 5});
  Matrix D = Matrix("A4", {5, 10});
  Matrix E = Matrix("A5", {10, 20});
  Matrix F = Matrix("A6", {20, 25});

  cout << "\n\n";
  return 0;
}
