#include "chain.h"
#include "llvm/Support/Casting.h"
#include <iostream>

bool Expr::isTransposeOf(shared_ptr<Expr> right) {
  if (auto t = llvm::dyn_cast_or_null<UnaryOp>(this)) {
    if (t->getKind() != UnaryOp::UnaryOpKind::TRANSPOSE)
      return false;
    // well how to do this in MLIR?
    return t->getChild().get() == right.get();
  }
  return false;
}
