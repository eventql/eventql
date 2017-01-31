/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#pragma once
#include <stdlib.h>
#include <vector>
#include <eventql/sql/runtime/ScratchMemory.h>

namespace csql {
class SValue;
class ScratchMemory;

struct VMRegister {
  void* data;
  size_t capacity;
};

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
    kInstructionType type;
    void* arg0;
    size_t argn;
    Instruction* next;
    Instruction* child;
    SFunction::VTable vtable;
    VMRegister retval;
  };

  struct Program {
    Program(
        Transaction* ctx,
        Instruction* entry,
        ScratchMemory&& static_storage,
        size_t dynamic_storage_size,
        SType return_type);

    ~Program();

    Transaction* ctx_;
    Instruction* entry_;
    ScratchMemory static_storage_;
    size_t dynamic_storage_size_;
    SType return_type_;
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

  static void evaluate(
      Transaction* ctx,
      const Program* program,
      int argc,
      void** argv,
      VMRegister* out);

  static void evaluateVector(
      Transaction* ctx,
      const Program* program,
      int argc,
      const SVector** argv,
      size_t vlen,
      SVector* out);

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
      void** argv);

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
      VMRegister* out);

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

  static void copyRegister(SType type, VMRegister* dst, void* src);

protected:

  static void evaluate(
      Transaction* ctx,
      const Program* program,
      Instance* instance,
      Instruction* expr,
      int argc,
      void** argv,
      VMRegister* out);

  static void accumulate(
      Transaction* ctx,
      const Program* program,
      Instance* instance,
      Instruction* expr,
      int argc,
      void** argv);

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

