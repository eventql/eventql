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
class Transaction;

struct VMStack {
  VMStack();
  char* data;
  char* top;
  char* limit;
};

namespace vm {

enum InstructionType {
  X_CALL_PURE,
  X_CALL_INSTANCE,
  X_LITERAL,
  X_INPUT,
  X_JUMP,
  X_CJUMP,
  X_RETURN
};

struct Instruction {
  Instruction(InstructionType _type, intptr_t _arg0);
  Instruction(InstructionType _type, intptr_t _arg0, SType _argt);
  InstructionType type;
  intptr_t arg0;
  SType argt;
};

struct EntryPoint {
  EntryPoint();
  EntryPoint(size_t offset);
  size_t offset;
};

struct Program {
  Program();
  std::vector<vm::Instruction> instructions;
  vm::EntryPoint method_call;
  vm::EntryPoint method_accumulate;
  ScratchMemory static_storage;
  size_t instance_storage_size;
  SType return_type;
  void (*instance_reset)(sql_txn*, void* self);
  void (*instance_init)(sql_txn*, void* self);
  void (*instance_free)(sql_txn*, void* self);
  void (*instance_merge)(sql_txn*, void* self, const void* other);
  void (*instance_savestate)(sql_txn*, const void* self, OutputStream* os);
  void (*instance_loadstate)(sql_txn*, void* self, InputStream* is);
};

static const size_t kStackBlockSize = 512 * 1024; // 512k

void growStack(VMStack* stack, size_t bytes);

} // namespace vm

class VM {
public:

  using Instance = void*;

  static void evaluate(
      Transaction* ctx,
      const vm::Program* program,
      vm::EntryPoint entrypoint,
      VMStack* stack,
      Instance* instance,
      int argc,
      void** argv);

  static void evaluateBoxed(
      Transaction* ctx,
      const vm::Program* program,
      vm::EntryPoint entrypoint,
      VMStack* stack,
      Instance* instance,
      int argc,
      const SValue* argv);

  static void evaluateVector(
      Transaction* ctx,
      const vm::Program* program,
      vm::EntryPoint entrypoint,
      VMStack* stack,
      Instance* instance,
      int argc,
      const SVector* argv,
      size_t vlen,
      SVector* out,
      const std::vector<bool>* filter = nullptr);

  static void evaluatePredicateVector(
      Transaction* ctx,
      const vm::Program* program,
      vm::EntryPoint entrypoint,
      VMStack* stack,
      Instance* instance,
      int argc,
      const SVector* argv,
      size_t vlen,
      std::vector<bool>* out,
      size_t* out_cardinality);

  static Instance allocInstance(
      Transaction* ctx,
      const vm::Program* program,
      ScratchMemory* scratch);

  static void freeInstance(
      Transaction* ctx,
      const vm::Program* program,
      Instance* instance);

  static void resetInstance(
      Transaction* ctx,
      const vm::Program* program,
      Instance* instance);

  static void mergeInstance(
      Transaction* ctx,
      const vm::Program* program,
      Instance* dst,
      const Instance* src);

  static void saveInstanceState(
      Transaction* ctx,
      const vm::Program* program,
      const Instance* instance,
      OutputStream* os);

  static void loadInstanceState(
      Transaction* ctx,
      const vm::Program* program,
      Instance* instance,
      InputStream* os);

protected:

  static void initProgram(
      Transaction* ctx,
      vm::Program* program,
      vm::Instruction* e);

  static void freeProgram(
      Transaction* ctx,
      const vm::Program* program,
      vm::Instruction* e);

  static void initInstance(
      Transaction* ctx,
      const vm::Program* program,
      vm::Instruction* e,
      Instance* instance);

  static void freeInstance(
      Transaction* ctx,
      const vm::Program* program,
      vm::Instruction* e,
      Instance* instance);

  static void resetInstance(
      Transaction* ctx,
      const vm::Program* program,
      vm::Instruction* e,
      Instance* instance);

  static void mergeInstance(
      Transaction* ctx,
      const vm::Program* program,
      vm::Instruction* e,
      Instance* dst,
      const Instance* src);

  static void saveInstance(
      Transaction* ctx,
      const vm::Program* program,
      vm::Instruction* e,
      const Instance* instance,
      OutputStream* os);

  static void loadInstance(
      Transaction* ctx,
      const vm::Program* program,
      vm::Instruction* e,
      Instance* instance,
      InputStream* is);

};

} // namespace csql

