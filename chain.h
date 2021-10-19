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

#ifndef MATRIX_CHAIN_UTILS_H
#define MATRIX_CHAIN_UTILS_H

#include <cassert>
#include <memory>
#include <vector>

#include <iostream>

namespace matrixchain {

using namespace std;

/// Generic expr of type BINARY, UNARY or OPERAND.
class Expr {
public:
  enum class ExprKind { BINARY, UNARY, OPERAND, NARY };
  enum class ExprProperty {
    UPPER_TRIANGULAR,
    LOWER_TRIANGULAR,
    SQUARE,
    SYMMETRIC,
    FULL_RANK,
    SPD
  };

private:
  const ExprKind kind;

protected:
  vector<Expr::ExprProperty> inferredProperties;

public:
  ExprKind getKind() const { return kind; }
  virtual void setProperties(vector<Expr::ExprProperty> properties) {
    assert(0 && "can set properties only for operands");
  };
  virtual ~Expr(){};

  virtual Expr *clone() const = 0;

  virtual bool isUpperTriangular() = 0;
  virtual bool isLowerTriangular() = 0;
  virtual bool isSquare() = 0;
  virtual bool isSymmetric() = 0;
  virtual bool isFullRank() = 0;
  virtual bool isSPD() = 0;

  bool isTransposeOf(Expr *right);

protected:
  Expr() = delete;
  Expr(ExprKind kind) : kind(kind), inferredProperties({}){};
};

/// Binary operation (i.e., MUL).
class BinaryOp : public Expr {
public:
  enum class BinaryOpKind { MUL };

private:
  Expr *childLeft;
  Expr *childRight;
  BinaryOpKind kind;

public:
  BinaryOp() = delete;
  ~BinaryOp() {
    delete childLeft;
    delete childRight;
  };

  BinaryOp(Expr *left, Expr *right, BinaryOpKind kind)
      : Expr(ExprKind::BINARY), childLeft(left), childRight(right),
        kind(kind){};
  BinaryOp(const BinaryOp &binaryOp) : Expr(binaryOp) {
    childLeft = binaryOp.getLeftChild()->clone();
    childRight = binaryOp.getRightChild()->clone();
    kind = binaryOp.getKind();
  }

  BinaryOpKind getKind() const { return kind; };
  Expr *getLeftChild() const { return childLeft; };
  Expr *getRightChild() const { return childRight; };
  BinaryOp *clone() const { return new BinaryOp(*this); };

  bool isUpperTriangular();
  bool isLowerTriangular();
  bool isSquare();
  bool isSymmetric();
  bool isFullRank();
  bool isSPD();

  static bool classof(const Expr *expr) {
    return expr->getKind() == ExprKind::BINARY;
  };
};

/*
/// N-ary operation (i.e., MUL)
class NaryOp : public Expr {
public:
  enum class NaryOpKind { MUL };

private:
  vector<unique_ptr<Expr>> children;
  NaryOpKind kind;

public:
  NaryOp() = delete;
  NaryOp(vector<Expr*> children)
      : Expr(ExprKind::NARY), children(children){};
  NaryOpKind getKind() { return kind; };
  vector<unique_ptr<Expr>> getChildren() { return children; };

  bool isUpperTriangular();
  bool isLowerTriangular();
  bool isSquare() { return false; };
  bool isSymmetric() { return false; };
  bool isFullRank() { return false; };
  bool isSPD() { return false; };

  static bool classof(const Expr *expr) {
    return expr->getKind() == ExprKind::NARY;
  };
};
*/

/// Unary operation like transpose or inverse.
class UnaryOp : public Expr {
public:
  enum class UnaryOpKind { TRANSPOSE, INVERSE };

private:
  Expr *child;
  UnaryOpKind kind;

public:
  UnaryOp() = delete;
  ~UnaryOp() { delete child; };
  UnaryOp(Expr *child, UnaryOpKind kind)
      : Expr(ExprKind::UNARY), child(child), kind(kind){};
  UnaryOp(const UnaryOp &unaryOp) : Expr(unaryOp) {
    child = unaryOp.getChild()->clone();
    kind = unaryOp.getKind();
  }

  Expr *getChild() const { return child; };
  UnaryOpKind getKind() const { return kind; };
  UnaryOp *clone() const { return new UnaryOp(*this); };

  bool isSquare();
  bool isSymmetric();
  bool isUpperTriangular();
  bool isLowerTriangular();
  bool isFullRank();
  bool isSPD();

  static bool classof(const Expr *expr) {
    return expr->getKind() == ExprKind::UNARY;
  };
};

/// Generic operand (i.e., matrix or vector).
class Operand : public Expr {
private:
  string name;
  vector<int> shape;

public:
  Operand() = delete;
  ~Operand(){};
  Operand(string name, vector<int> shape)
      : Expr(ExprKind::OPERAND), name(name), shape(shape){};
  Operand(const Operand &) = default;

  string getName() { return name; };
  vector<int> getShape() { return shape; };
  vector<Expr::ExprProperty> getProperties() { return inferredProperties; };
  void setProperties(vector<Expr::ExprProperty> properties) {
    inferredProperties = properties;
  };
  Operand *clone() const {
    cout << "cloned operand\n";
    return new Operand(*this);
  };

  bool isUpperTriangular();
  bool isLowerTriangular();
  bool isSquare();
  bool isSymmetric();
  bool isFullRank();
  bool isSPD();

  static bool classof(const Expr *expr) {
    return expr->getKind() == ExprKind::OPERAND;
  };
};

} // end namespace matrixchain

using namespace std;
using namespace matrixchain;

namespace {
template <typename... Args>
vector<typename std::common_type<Args...>::type> varargToVector(Args... args) {
  vector<typename std::common_type<Args...>::type> result;
  result.reserve(sizeof...(Args));
  for (auto arg :
       {static_cast<typename std::common_type<Args...>::type>(args)...}) {
    result.emplace_back(arg);
  }
  return result;
}
} // end namespace

// Exposed methods.
void walk(Expr *node, int level = 0);
Expr *inv(Expr *child);
Expr *trans(Expr *child);
long getMCPFlops(Expr *expr);

namespace details {
Expr *binaryMul(Expr *left, Expr *right);
}

// Exposed method: Variadic Mul.
template <typename Arg, typename... Args> Expr *mul(Arg arg, Args... args) {
  auto operands = varargToVector<Expr *>(arg, args...);
  assert(operands.size() >= 1 && "one or more mul");
  if (operands.size() == 1)
    return operands[0];
  auto result = operands[0];
  for (size_t i = 1; i < operands.size(); i++)
    result = details::binaryMul(result, operands[i]);
  return result;
}

// Exposed for debug only.
// void getKernelCostTopLevelExpr(Expr *node, long &cost);
// void getKernelCostFullExpr(Expr *node, long &cost);

#endif
