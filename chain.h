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
#include <set>
#include <vector>

namespace details {

using namespace std;

// forward decl.
class Expr;

/// Scoped context to handle deallocation.
class ScopedContext {
public:
  ScopedContext() { ScopedContext::getCurrentScopedContext() = this; };
  ~ScopedContext();
  ScopedContext(const ScopedContext &) = delete;
  ScopedContext &operator=(const ScopedContext &) = delete;

  void insert(Expr *expr) { liveRefs.insert(expr); }
  void print();
  static ScopedContext *&getCurrentScopedContext();

private:
  std::set<Expr *> liveRefs;
};

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
  // TODO: check me.
  virtual ~Expr() = default;
  virtual void inferProperties() = 0;

  virtual bool isUpperTriangular() = 0;
  virtual bool isLowerTriangular() = 0;
  virtual bool isSquare() = 0;
  virtual bool isSymmetric() = 0;
  virtual bool isFullRank() = 0;
  virtual bool isSPD() = 0;

  bool isTransposeOf(const Expr *right);
  bool isSame(const Expr *right);

protected:
  Expr() = delete;
  Expr(ExprKind kind) : kind(kind), inferredProperties({}){};
};

/// CRTP to inject
template <class T> struct AutoRef : public Expr {

private:
  friend T;

  AutoRef(Expr::ExprKind kind) : Expr(kind) {
    auto ctx = ScopedContext::getCurrentScopedContext();
    assert(ctx != nullptr && "ctx not available");
    ctx->insert(static_cast<T *>(this));
  }
};

/// Binary operation (i.e., MUL).
class BinaryOp : public AutoRef<BinaryOp> {
public:
  enum class BinaryOpKind { MUL };

private:
  Expr *childLeft;
  Expr *childRight;
  BinaryOpKind kind;

public:
  BinaryOp() = delete;
  BinaryOp(Expr *left, Expr *right, BinaryOpKind kind)
      : AutoRef(ExprKind::BINARY), childLeft(left), childRight(right),
        kind(kind){};
  BinaryOpKind getKind() const { return kind; };
  void inferProperties();
  Expr *getLeftChild() const { return childLeft; };
  Expr *getRightChild() const { return childRight; };
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

/// N-ary operation (i.e., MUL)
class NaryOp : public Expr {
public:
  enum class NaryOpKind { MUL };

private:
  vector<Expr *> children;
  NaryOpKind kind;

public:
  NaryOp() = delete;
  NaryOp(vector<Expr *> children) : Expr(ExprKind::NARY), children(children){};
  NaryOpKind getKind() { return kind; };
  vector<Expr *> getChildren() { return children; };
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

/// Unary operation like transpose or inverse.
class UnaryOp : public AutoRef<UnaryOp> {
public:
  enum class UnaryOpKind { TRANSPOSE, INVERSE };

private:
  Expr *child;
  UnaryOpKind kind;

public:
  UnaryOp() = delete;
  UnaryOp(Expr *child, UnaryOpKind kind)
      : AutoRef(ExprKind::UNARY), child(child), kind(kind){};
  void inferProperties();
  Expr *getChild() const { return child; };
  UnaryOpKind getKind() const { return kind; };
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

Expr *binaryMul(Expr *left, Expr *right);

} // end namespace details.

namespace matrixchain {

using namespace details;

/// Generic operand (i.e., matrix or vector).
class Operand : public AutoRef<Operand> {
private:
  string name;
  vector<int> shape;

public:
  Operand() = delete;
  Operand(string name, vector<int> shape)
      : AutoRef(ExprKind::OPERAND), name(name), shape(shape){};
  string getName() { return name; };
  vector<int> getShape() { return shape; };
  vector<Expr::ExprProperty> getProperties() { return inferredProperties; };
  void setProperties(vector<Expr::ExprProperty> properties) {
    inferredProperties = properties;
  };
  void inferProperties(){};
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
void getKernelCostTopLevelExpr(Expr *node, long &cost);
void getKernelCostFullExpr(Expr *node, long &cost);

#endif
