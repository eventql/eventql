// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <cortex-flow/Api.h>
#include <cortex-flow/ir/Value.h>
#include <cortex-flow/ir/InstructionVisitor.h>
#include <cortex-flow/vm/Instruction.h>
#include <cortex-flow/vm/MatchClass.h>
#include <cortex-flow/vm/Signature.h>
#include <cortex-base/net/IPAddress.h>
#include <cortex-base/net/Cidr.h>
#include <cortex-base/RegExp.h>

#include <string>
#include <vector>

namespace cortex {
namespace flow {

class Instr;
class TerminateInstr;
class IRHandler;
class IRBuilder;

/**
 * An SSA based instruction basic block.
 *
 * @see Instr, IRHandler, IRBuilder
 */
class CORTEX_FLOW_API BasicBlock : public Value {
 public:
  explicit BasicBlock(const std::string& name);
  ~BasicBlock();

  IRHandler* parent() const { return parent_; }
  void setParent(IRHandler* handler) { parent_ = handler; }

  /*!
   * Retrieves the last terminating instruction in this basic block.
   *
   * This instruction must be a termination instruction, such as
   * a branching instruction or a handler terminating instruction.
   *
   * @see BrInstr, CondBrInstr, MatchInstr, RetInstr
   */
  TerminateInstr* getTerminator() const;

  /**
   * Retrieves the linear ordered list of instructions of instructions in this
   * basic block.
   */
  const std::vector<Instr*>& instructions() const { return code_; }

  /**
   * Appends a new instruction, \p instr, to this basic block.
   *
   * The basic block will take over ownership of the given instruction.
   */
  void push_back(Instr* instr);

  /**
   * Removes given instruction from this basic block.
   *
   * The basic block will pass ownership of the given instruction to the caller.
   * That means, the caller has to either delete \p childInstr or transfer it to
   *another basic block.
   *
   * @see push_back()
   */
  Instr* remove(Instr* childInstr);

  /**
   * Merges given basic block's instructions into this ones end.
   *
   * The passed basic block's instructions will not be touched.
   */
  void merge_back(BasicBlock* bb);

  /**
   * Moves this basic block after the other basic block, \p otherBB.
   *
   * @param otherBB the future prior basic block.
   *
   * In a function, all basic blocks (starting from the entry block)
   * will be aligned linear into the execution segment.
   *
   * This function moves the given basic block directly after
   * the other basic block, \p otherBB.
   *
   * @see moveBefore()
   */
  void moveAfter(BasicBlock* otherBB);

  /**
   * Moves this basic block before the other basic block, \p otherBB.
   *
   * @see moveAfter()
   */
  void moveBefore(BasicBlock* otherBB);

  /**
   * Tests whether or not given block is straight-line located after this block.
   *
   * @retval true \p otherBB is straight-line located after this block.
   * @retval false \p otherBB is not straight-line located after this block.
   *
   * @see moveAfter()
   */
  bool isAfter(const BasicBlock* otherBB) const;

  /**
   * Links given \p successor basic block to this predecessor.
   *
   * @param successor the basic block to link as an successor of this basic
   *block.
   *
   * This will also automatically link this basic block as
   * future predecessor of the \p successor.
   *
   * @see unlinkSuccessor()
   * @see successors(), predecessors()
   */
  void linkSuccessor(BasicBlock* successor);

  /**
   * Unlink given \p successor basic block from this predecessor.
   *
   * @see linkSuccessor()
   * @see successors(), predecessors()
   */
  void unlinkSuccessor(BasicBlock* successor);

  /** Retrieves all predecessors of given basic block. */
  std::vector<BasicBlock*>& predecessors() { return predecessors_; }

  /** Retrieves all uccessors of the given basic block. */
  std::vector<BasicBlock*>& successors() { return successors_; }
  const std::vector<BasicBlock*>& successors() const { return successors_; }

  /** Retrieves all dominators of given basic block. */
  std::vector<BasicBlock*> dominators();

  /** Retrieves all immediate dominators of given basic block. */
  std::vector<BasicBlock*> immediateDominators();

  void dump() override;

  /**
   * Performs sanity checks on internal data structures.
   *
   * This call does not return any success or failure as every failure is
   *considered fatal
   * and will cause the program to exit with diagnostics as this is most likely
   *caused by
   * an application programming error.
   *
   * @note This function is automatically invoked by IRHandler::verify()
   *
   * @see IRHandler::verify()
   */
  void verify();

 private:
  void collectIDom(std::vector<BasicBlock*>& output);

 private:
  IRHandler* parent_;
  std::vector<Instr*> code_;
  std::vector<BasicBlock*> predecessors_;
  std::vector<BasicBlock*> successors_;

  friend class IRBuilder;
  friend class Instr;
};

}  // namespace flow
}  // namespace cortex
