#include "llvm/Support/Casting.h"
#include <iostream>
#include <memory>
#include <set>

using namespace std;

namespace details {

// forward.
class Expr;
class Operand;
class UnaryOp;
class BinaryOp;

class ScopedContext {
public:
  ScopedContext() { ScopedContext::getCurrentScopedContext() = this; };
  ~ScopedContext();
  ScopedContext(const ScopedContext &) = delete;
  ScopedContext &operator=(const ScopedContext &) = delete;

  void print() {
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

  void insert(Expr *expr) { liveRefs.insert(expr); }

  static ScopedContext *&getCurrentScopedContext();

private:
  std::set<Expr *> liveRefs;
};

ScopedContext *&ScopedContext::getCurrentScopedContext() {
  thread_local ScopedContext *context = nullptr;
  return context;
}

class Expr {
public:
  enum class ExprKind { BINARY, UNARY, OPERAND };

private:
  const ExprKind kind;

public:
  ExprKind getKind() const { return kind; };
  Expr() = delete;
  Expr(ExprKind kind) : kind(kind){};
  virtual ~Expr() { cout << "delete expr\n"; };

  static ScopedContext *&getCurrentScopedContext();
};

template <class T> struct AutoRef : public Expr {

private:
  friend T;

  AutoRef(Expr::ExprKind kind) : Expr(kind) {
    auto ctx = ScopedContext::getCurrentScopedContext();
    assert(ctx != nullptr && "ctx not available");
    ctx->insert(static_cast<T *>(this));
  }
};

class BinaryOp : public AutoRef<BinaryOp> {
public:
  BinaryOp() = delete;
  BinaryOp(Expr *left, Expr *right)
      : leftChild(left), rightChild(right), AutoRef(Expr::ExprKind::BINARY){};
  ~BinaryOp() { cout << "delete binaryOp\n"; };

  static bool classof(const Expr *expr) {
    return expr->getKind() == ExprKind::BINARY;
  };

private:
  Expr *leftChild;
  Expr *rightChild;
};

class UnaryOp : public AutoRef<UnaryOp> {
public:
  UnaryOp() = delete;
  UnaryOp(Expr *child) : child(child), AutoRef(Expr::ExprKind::UNARY){};
  ~UnaryOp() { cout << "delete unary\n"; };

  static bool classof(const Expr *expr) {
    return expr->getKind() == ExprKind::UNARY;
  }

private:
  Expr *child;
};

class Operand : public AutoRef<Operand> {
public:
  enum class OperandKind { MATRIX, VECTOR };

public:
  Operand(string name) : name(name), AutoRef(ExprKind::OPERAND){};
  Operand(const Operand &operand) = default;
  ~Operand() { cout << "delete operand\n"; };

  static bool classof(const Expr *expr) {
    return expr->getKind() == ExprKind::OPERAND;
  };

private:
  string name;
};

ScopedContext::~ScopedContext() {
  for (auto expr : liveRefs)
    delete expr;
}

BinaryOp *mulImpl(Expr *left, Expr *right) { return new BinaryOp(left, right); }
UnaryOp *transImpl(Expr *child) { return new UnaryOp(child); }

} // end namespace details

namespace matrixchain {

class Matrix : public details::Operand {
  using details::Operand::Operand;
};

details::Expr *mul(details::Expr *left, details::Expr *right) {
  return details::mulImpl(left, right);
}

details::Expr *trans(details::Expr *child) { return details::transImpl(child); }

} // end namespace matrixchain

int main() {

  using namespace matrixchain;
  details::ScopedContext ctx;

  Matrix *A(new Matrix("A"));
  Matrix *B(new Matrix("B"));
  Matrix *C(new Matrix("C"));

  {
    details::ScopedContext ctx;

    Matrix *D(new Matrix("D"));
    details::Expr *F = mul(D, trans(D));

    ctx.print();
  }

  ctx.print();

  return 0;
}
