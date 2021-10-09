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

namespace matrixchain {

using namespace std;

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
  virtual bool isLowerTriangular() = 0;

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
  bool isUpperTriangular();
  bool isLowerTriangular();
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
  bool isUpperTriangular();
  bool isLowerTriangular();
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
  bool isUpperTriangular();
  bool isLowerTriangular();
  static bool classof(const Expr *expr) {
    return expr->getKind() == ExprKind::OPERAND;
  };
};

} // end namespace matrixchain

using namespace std;
using namespace matrixchain;

// Exposed methods.
void walk(shared_ptr<Expr> node, int level = 0);
shared_ptr<Expr> mul(shared_ptr<Expr> left, shared_ptr<Expr> right);
shared_ptr<Expr> inv(shared_ptr<Expr> child);
shared_ptr<Expr> trans(shared_ptr<Expr> child);

long getMCPFlops(shared_ptr<Expr> &expr);

#endif
