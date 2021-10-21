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
#include <iostream>
#include <limits>

using namespace matrixchain;
using namespace details;

void UnaryOp::inferProperties() { return; }

void BinaryOp::inferProperties() { return; }

ScopedContext *&ScopedContext::getCurrentScopedContext() {
  thread_local ScopedContext *context = nullptr;
  return context;
}

ScopedContext::~ScopedContext() {
  for (auto expr : liveRefs)
    delete expr;
}

void ScopedContext::print() {
  cout << "#live refs: " << liveRefs.size() << "\n";
  int operands = 0;
  int unaries = 0;
  int binaries = 0;
  for (Expr *expr : liveRefs) {
    if (llvm::isa<Operand>(expr))
      operands++;
    if (llvm::isa<UnaryOp>(expr))
      unaries++;
    if (llvm::isa<BinaryOp>(expr))
      binaries++;
  }
  cout << "#operands : " << operands << "\n";
  cout << "#unaries : " << unaries << "\n";
  cout << "#binaries : " << binaries << "\n";
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
    case Expr::ExprProperty::SQUARE:
      cout << "SQUARE";
      break;
    case Expr::ExprProperty::SYMMETRIC:
      cout << "SYMMETRIC";
      break;
    case Expr::ExprProperty::FULL_RANK:
      cout << "FULL_RANK";
      break;
    case Expr::ExprProperty::SPD:
      cout << "SPD";
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
void walk(const Expr *node, int level) {
  if (node) {
    if (auto binaryOp = llvm::dyn_cast_or_null<BinaryOp>(node)) {
      switch (binaryOp->getKind()) {
      case BinaryOp::BinaryOpKind::MUL:
        cout << string(level, ' ') << "(*\n";
        break;
      default:
        cout << "UNK";
      }
      for (auto child : binaryOp->getChildren()) {
        walk(child, level + LEVEL_SPACES);
        cout << " \n";
      }
    } // binaryOp
    if (auto unaryOp = llvm::dyn_cast_or_null<UnaryOp>(node)) {
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
    if (auto operand = llvm::dyn_cast_or_null<Operand>(node)) {
      cout << string(level, ' ') << operand->getName() << " [";
      printProperties(operand->getProperties());
      cout << "] [";
      printShape(operand->getShape());
      cout << "]";
    } // operand
  }
}

/// Multiply two expressions.
Expr *details::binaryMul(vector<Expr *> children) {
  return new BinaryOp(children, BinaryOp::BinaryOpKind::MUL);
}

/// invert an expression.
Expr *inv(Expr *child) {
  assert(child && "child expr must be non null");
  return new UnaryOp(child, UnaryOp::UnaryOpKind::INVERSE);
}

/// transpose an expression.
Expr *trans(Expr *child) {
  assert(child && "child expr must be non null");
  return new UnaryOp(child, UnaryOp::UnaryOpKind::TRANSPOSE);
}

static vector<long> getPVector(vector<Expr *> exprs) {
  vector<long> pVector;
  for (auto expr : exprs) {
    Operand *operand = nullptr;
    if (auto unaryOp = llvm::dyn_cast_or_null<UnaryOp>(expr))
      operand = llvm::dyn_cast_or_null<Operand>(unaryOp->getChild());
    else
      operand = llvm::dyn_cast_or_null<Operand>(expr);
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
                               size_t j, vector<Expr *> operands) {
  if (i == j) {
    cout << " ";
    Operand *operand = nullptr;
    if (auto unaryOp = llvm::dyn_cast_or_null<UnaryOp>(operands[i - 1]))
      operand = llvm::dyn_cast_or_null<Operand>(unaryOp->getChild());
    else
      operand = llvm::dyn_cast_or_null<Operand>(operands[i - 1]);
    assert(operand && "must be non null");
    if (llvm::isa<UnaryOp>(operands[i - 1]))
      cout << "u(" << operand->getName() << ")";
    else
      cout << operand->getName();
    cout << "  ";
  } else {
    cout << "(";
    printOptimalParens(s, i, s[i][j], operands);
    printOptimalParens(s, s[i][j] + 1, j, operands);
    cout << ")";
  }
}

static void collectOperandsImpl(Expr *node, vector<Expr *> &operands) {
  if (node) {
    if (auto binaryOp = llvm::dyn_cast_or_null<BinaryOp>(node)) {
      for (auto child : binaryOp->getChildren()) {
        collectOperandsImpl(child, operands);
      }
    }
    if (llvm::isa<UnaryOp>(node) || llvm::isa<Operand>(node)) {
      assert(node != nullptr && "must be non-null");
      operands.push_back(node);
    }
  }
}

static vector<Expr *> collectOperands(Expr *expr) {
  vector<Expr *> operands;
  collectOperandsImpl(expr, operands);
  return operands;
}

#if DEBUG
static void print(vector<vector<Expr *>> &tmps, bool bitLayout = false) {
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

// TODO: n-ary how to handle? Do we need to?
pair<long, long> getKernelCostImpl(Expr *node, long &cost, bool fullTree) {
  if (node) {
    if (auto binaryOp = llvm::dyn_cast_or_null<BinaryOp>(node)) {
      auto children = binaryOp->getChildren();
      assert(children.size() == 2);
      pair<long, long> left = getKernelCostImpl(children[0], cost, fullTree);
      pair<long, long> right = getKernelCostImpl(children[1], cost, fullTree);
      // note this cost must be the cost of the top level expr
      // not the cost of the tree.
      // GEMM by default adjust later on.
      auto currentCost = left.first * left.second * right.second * 2;
      // TRMM TODO: must be square the other?
      if (children[0]->isLowerTriangular())
        currentCost >>= 1;
      // SYMM TODO: must be square the other?
      else if (children[0]->isSymmetric())
        currentCost >>= 1;

      if (fullTree)
        cost += currentCost;
      else
        cost = currentCost;

      return {left.first, right.second};
    }
    if (auto unaryOp = llvm::dyn_cast_or_null<UnaryOp>(node)) {
      return getKernelCostImpl(unaryOp->getChild(), cost, fullTree);
    }
    if (auto operand = llvm::dyn_cast_or_null<Operand>(node)) {
      auto shape = operand->getShape();
      assert(shape.size() == 2 && "must be 2d");
      return {shape[0], shape[1]};
    }
  }
  return {0, 0};
}

void getKernelCostFullExpr(Expr *node, long &cost) {
  (void)getKernelCostImpl(node, cost, true);
}

void getKernelCostTopLevelExpr(Expr *node, long &cost) {
  (void)getKernelCostImpl(node, cost, false);
}

struct ResultMCP {
  vector<vector<long>> m;
  vector<vector<long>> s;
};

ResultMCP runMCP(Expr *expr) {
#if DEBUG
  cout << "Starting point\n";
  walk(expr);
  cout << "\n\n";
#endif
  vector<Expr *> operands = collectOperands(expr);
  vector<long> pVector = getPVector(operands);
  const size_t n = pVector.size();
  vector<vector<long>> m(n, vector<long>(n, std::numeric_limits<long>::max()));
  vector<vector<long>> s(n, vector<long>(n, std::numeric_limits<long>::max()));

  // store symbolic temporary variables representing sub-chains.
  vector<vector<Expr *>> tmps(n, vector<Expr *>(n, nullptr));

  for (size_t i = 0; i < n - 1; i++)
    tmps[i + 1][i + 1] = operands.at(i);

#if DEBUG
  cout << "\n\n-before-tmps-\n";
  print(tmps, true);
#endif

  for (size_t i = 0; i < n; i++)
    m[i][i] = 0;

  size_t j = 0;
  long q = 0;
  for (size_t l = 2; l < n; l++) {
    for (size_t i = 1; i < n - l + 1; i++) {
      j = i + l - 1;
      m[i][j] = std::numeric_limits<long>::max();
      for (size_t k = i; k <= j - 1; k++) {

        auto tmpexpr = mul(tmps[i][k], tmps[k + 1][j]);
#if DEBUG
        cout << "---\n";
        walk(tmpexpr);
        cout << "\n---\n\n";
#endif
        long cost = 0;
        getKernelCostTopLevelExpr(tmpexpr, cost);
        // long cost = 2 * pVector.at(i - 1) * pVector.at(k) * pVector.at(j);
        q = m[i][k] + m[k + 1][j] + cost;
        if (q < m[i][j]) {
          tmps[i][j] = mul(tmps[i][k], tmps[k + 1][j]);
          tmps[i][j]->inferProperties();
          m[i][j] = q;
          s[i][j] = k;
        }
      }
    }
  }

#if DEBUG
  cout << "\n\n-after-tmps-\n";
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

long getMCPFlops(Expr *expr) {
  ResultMCP result = runMCP(expr);
  auto m = result.m;
#if DEBUG
  cout << "FLOPS: " << m[1][m.size() - 1] << "\n";
#endif
  return m[1][m.size() - 1];
}
