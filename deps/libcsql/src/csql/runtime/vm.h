/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stdlib.h>
#include <vector>
#include <csql/runtime/ScratchMemory.h>

namespace csql {
class SValue;
class ScratchMemory;

class VM {
public:

  enum kInstructionType {
    X_CALL_PURE,
    X_CALL_AGGREGATE,
    X_LITERAL,
    X_INPUT,
    X_IF,
    X_REGEX,
    X_LIKE
  };

  struct Instruction {
    Instruction() : vtable{ .t_pure = nullptr } {}
    kInstructionType type;
    void* arg0;
    size_t argn;
    Instruction* next;
    Instruction* child;
    union {
      PureFunction t_pure;
      AggregateFunction t_aggregate;
    } vtable;
  };

  struct Program {
    Program(
        Transaction* ctx,
        Instruction* entry,
        ScratchMemory&& static_storage,
        size_t dynamic_storage_size);

    ~Program();

    Transaction* ctx_;
    Instruction* entry_;
    ScratchMemory static_storage_;
    size_t dynamic_storage_size_;
    bool has_aggregate_;
  };

  struct Instance {
    void* scratch;
  };

  static void evaluate(
      Transaction* ctx,
      const Program* program,
      int argc,
      const SValue* argv,
      SValue* out);

  static Instance allocInstance(
      Transaction* ctx,
      const Program* program,
      ScratchMemory* scratch);

  static void freeInstance(
      Transaction* ctx,
      const Program* program,
      Instance* instance);

  static void accumulate(
      Transaction* ctx,
      const Program* program,
      Instance* instance,
      int argc,
      const SValue* argv);

  static void result(
      Transaction* ctx,
      const Program* program,
      const Instance* instance,
      SValue* out);

  static void reset(
      Transaction* ctx,
      const Program* program,
      Instance* instance);

  // rename to mergeStat
  static void merge(
      Transaction* ctx,
      const Program* program,
      Instance* dst,
      const Instance* src);

  static void saveState(
      Transaction* ctx,
      const Program* program,
      const Instance* instance,
      OutputStream* os);

  static void loadState(
      Transaction* ctx,
      const Program* program,
      Instance* instance,
      InputStream* os);

protected:

  static void evaluate(
      Transaction* ctx,
      const Program* program,
      Instance* instance,
      Instruction* expr,
      int argc,
      const SValue* argv,
      SValue* out);

  static void accumulate(
      Transaction* ctx,
      const Program* program,
      Instance* instance,
      Instruction* expr,
      int argc,
      const SValue* argv);

  static void initInstance(
      Transaction* ctx,
      const Program* program,
      Instruction* e,
      Instance* instance);

  static void freeInstance(
      Transaction* ctx,
      const Program* program,
      Instruction* e,
      Instance* instance);

  static void resetInstance(
      Transaction* ctx,
      const Program* program,
      Instruction* e,
      Instance* instance);

  static void initProgram(
      Transaction* ctx,
      Program* program,
      Instruction* e);

  static void freeProgram(
      Transaction* ctx,
      const Program* program,
      Instruction* e);

  static void mergeInstance(
      Transaction* ctx,
      const Program* program,
      Instruction* e,
      Instance* dst,
      const Instance* src);

  static void saveInstance(
      Transaction* ctx,
      const Program* program,
      Instruction* e,
      const Instance* instance,
      OutputStream* os);

  static void loadInstance(
      Transaction* ctx,
      const Program* program,
      Instruction* e,
      Instance* instance,
      InputStream* is);

};

} // namespace csql

