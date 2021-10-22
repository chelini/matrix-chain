/*
Copyright 2021 Lorenzo Chelini <l.chelini@icloud.com> or
<lorenzo.chelini@huawei.com>

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
#include <iostream>

bool Expr::isTransposeOf(const Expr *right) {
  // trans(left), right with *left == *right
  if (auto t = llvm::dyn_cast_or_null<UnaryOp>(this)) {
    if (t->getKind() != UnaryOp::UnaryOpKind::TRANSPOSE)
      return false;
    return t->getChild() == right;
  }
  // left, trans(right) with *left == *right
  if (auto t = llvm::dyn_cast_or_null<UnaryOp>(right)) {
    if (t->getKind() != UnaryOp::UnaryOpKind::TRANSPOSE)
      return false;
    return t->getChild() == this;
  }
  return false;
}

static bool isSameImpl(const Expr *tree1, const Expr *tree2) {
  if (!tree1 && !tree2)
    return true;

  if (tree1 && tree2) {
    if (tree1->getKind() != tree2->getKind())
      return false;
    // pt comparison for operands.
    if (llvm::isa<Operand>(tree1)) {
      return tree1 == tree2;
    }
    // unary.
    if (llvm::isa<UnaryOp>(tree1) && llvm::isa<UnaryOp>(tree2)) {
      const UnaryOp *tree1Op = llvm::dyn_cast_or_null<UnaryOp>(tree1);
      const UnaryOp *tree2Op = llvm::dyn_cast_or_null<UnaryOp>(tree2);
      return isSameImpl(tree1Op->getChild(), tree2Op->getChild());
    }
    // binary.
    if (llvm::isa<NaryOp>(tree1) && llvm::isa<NaryOp>(tree2)) {
      const NaryOp *tree1Op = llvm::dyn_cast_or_null<NaryOp>(tree1);
      const NaryOp *tree2Op = llvm::dyn_cast_or_null<NaryOp>(tree2);
      // TODO: fix
      return isSameImpl(tree1Op->getChildren()[0], tree2Op->getChildren()[0]) &&
             isSameImpl(tree1Op->getChildren()[1], tree2Op->getChildren()[1]);
    }
  }
  return false;
}

Expr *Operand::getNormalForm() { return this; }

Expr *NaryOp::getNormalForm() {
  // TODO: What is the normal form for mul?
  assert(0 && "not available");
  return nullptr;
}

Expr *UnaryOp::getNormalForm() {
  Expr *child = this->getChild();
  if (NaryOp *maybeMul = llvm::dyn_cast_or_null<NaryOp>(child)) {
    vector<Expr *> normalFormOperands;
    auto children = maybeMul->getChildren();
    int size = children.size();
    for (int i = size - 1; i >= 0; i--)
      normalFormOperands.push_back(trans(children.at(i)->getNormalForm()));
    return binaryMul(normalFormOperands);
  }
  assert(0 && "unreachable");
  return nullptr;
}

// return a brand new expr.
static Expr *getCanonicalForm(const Expr *tree) { return nullptr; }

bool Expr::isSame(const Expr *right) {
  if (isSameImpl(this, right))
    return true;
  Expr *canonicalForm = getCanonicalForm(this);
  if (isSameImpl(canonicalForm, right))
    return true;
  return false;
}
