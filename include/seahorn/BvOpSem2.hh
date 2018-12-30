#pragma once

#include "seahorn/Analysis/CanFail.hh"
#include "seahorn/OperationalSemantics.hh"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"
#include "llvm/Pass.h"

#include <boost/container/flat_set.hpp>

namespace llvm {
class GetElementPtrInst;
}

namespace seahorn {
namespace details {
class Bv2OpSemContext;
}
/**
   Bit-precise operational semantics for LLVM (take 2)

   Fairly accurate representation of LLVM semantics without
   considering undefined behaviour. Most operators are mapped
   directly to their logical equivalent SMT-LIB representation.

   Memory is modelled by arrays.
 */
class Bv2OpSem : public OperationalSemantics {
  Pass &m_pass;
  TrackLevel m_trackLvl;

  const DataLayout *m_td;
  const TargetLibraryInfo *m_tli;
  const CanFail *m_canFail;

public:
  Bv2OpSem(ExprFactory &efac, Pass &pass, const DataLayout &dl,
           TrackLevel trackLvl = MEM);

  Bv2OpSem(const Bv2OpSem &o);

  const DataLayout &getTD() {
    assert(m_td);
    return *m_td;
  }

  /// \brief Evaluates constant expression into a value
  Optional<GenericValue> getConstantValue(const Constant *C);

  OpSemContextPtr mkContext(SymStore &values, ExprVector &side) override;

  /// \brief Executes one intra-procedural instructions in the
  /// current context Assumes that current instruction is not a
  /// branch Returns true if instruction was executed and false if
  /// there is no suitable instruction found
  bool intraStep(seahorn::details::Bv2OpSemContext &C);
  /// \brief Executes one intra-procedural branch instruction in the
  /// current context. Assumes that current instruction is a branch
  void intraBr(seahorn::details::Bv2OpSemContext &C, const BasicBlock &dst);

  /// \brief Execute all PHINode instructions of the current basic block
  /// \brief assuming that control flows from previous basic block
  void intraPhi(seahorn::details::Bv2OpSemContext &C);

  Expr errorFlag(const BasicBlock &BB) override;

  void exec(const BasicBlock &bb, OpSemContext &_ctx) override {
    exec(bb, ctx(_ctx));
  }
  void execPhi(const BasicBlock &bb, const BasicBlock &from,
               OpSemContext &_ctx) override {
    execPhi(bb, from, ctx(_ctx));
  }
  void execEdg(const BasicBlock &src, const BasicBlock &dst,
               OpSemContext &_ctx) override {
    execEdg(src, dst, ctx(_ctx));
  }
  void execBr(const BasicBlock &src, const BasicBlock &dst,
              OpSemContext &_ctx) override {
    execBr(src, dst, ctx(_ctx));
  }

protected:
  void exec(const BasicBlock &bb, seahorn::details::Bv2OpSemContext &ctx);
  void execPhi(const BasicBlock &bb, const BasicBlock &from,
               seahorn::details::Bv2OpSemContext &ctx);
  void execEdg(const BasicBlock &src, const BasicBlock &dst,
               seahorn::details::Bv2OpSemContext &ctx);
  void execBr(const BasicBlock &src, const BasicBlock &dst,
              seahorn::details::Bv2OpSemContext &ctx);

public:
  /**
     \brief Returns a concrete representation of a given symbolic
            expression. Assumes that the input expression has
            concrete representation.
   */
  const Value &conc(Expr v) const override;

  /// \brief Indicates whether an instruction/value is skipped by
  /// the semantics. An instruction is skipped means that, from the
  /// perspective of the semantics, the instruction does not
  /// exist. It is not executed, has no effect on the execution
  /// context, and no instruction that is not skipped depends on it
  bool isSkipped(const Value &v) const;

  bool isTracked(const Value &v) const override { return !isSkipped(v); }

  /// \brief Returns true if the given expression is a symbolic register
  bool isSymReg(Expr v) override { llvm_unreachable(nullptr); }
  bool isSymReg(Expr v, seahorn::details::Bv2OpSemContext &ctx);

  Expr mkSymbReg(const Value &v, OpSemContext &_ctx) override;

  Expr getOperandValue(const Value &v, seahorn::details::Bv2OpSemContext &ctx);
  Expr lookup(SymStore &s, const Value &v) { llvm_unreachable(nullptr); }

  using gep_type_iterator = generic_gep_type_iterator<>;
  Expr symbolicIndexedOffset(gep_type_iterator it, gep_type_iterator end,
                             seahorn::details::Bv2OpSemContext &ctx);

  unsigned storageSize(const llvm::Type *t) const;
  unsigned fieldOff(const StructType *t, unsigned field) const;

  uint64_t sizeInBits(const llvm::Value &v) const;
  uint64_t sizeInBits(const llvm::Type &t) const;
  unsigned pointerSizeInBits() const;

  /// Reports (and records) an instruction as skipped by the semantics
  void skipInst(const Instruction &inst,
                seahorn::details::Bv2OpSemContext &ctx);
  /// Reports (and records) an instruction as not being handled by
  /// the semantics
  void unhandledInst(const Instruction &inst,
                     seahorn::details::Bv2OpSemContext &ctx);
  void unhandledValue(const Value &v, seahorn::details::Bv2OpSemContext &ctx);

private:
  static seahorn::details::Bv2OpSemContext &ctx(OpSemContext &_ctx);
};
} // namespace seahorn
