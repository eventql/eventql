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

struct VMStack {
  VMStack();
  char* data;
  size_t size;
  char* top;
};

void resizeStack(VMStack* stack);

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
    SType rettype;
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
    Instruction* method_call;
    Instruction* method_aggr_acc;
    Instruction* method_aggr_get;
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
      const Instruction* entrypoint,
      VMStack* stack,
      Instance* instance,
      int argc,
      void** argv);

  static void evaluateBoxed(
      Transaction* ctx,
      const Program* program,
      const Instruction* entrypoint,
      VMStack* stack,
      Instance* instance,
      int argc,
      const SValue* argv);

  static void evaluateVector(
      Transaction* ctx,
      const Program* program,
      const Instruction* entrypoint,
      VMStack* stack,
      Instance* instance,
      int argc,
      const SVector* argv,
      size_t vlen,
      SVector* out,
      const std::vector<bool>* filter = nullptr);

  static void evaluatePredicateVector(
      Transaction* ctx,
      const Program* program,
      const Instruction* entrypoint,
      VMStack* stack,
      Instance* instance,
      int argc,
      const SVector* argv,
      size_t vlen,
      std::vector<bool>* out,
      size_t* out_cardinality);

  static Instance allocInstance(
      Transaction* ctx,
      const Program* program,
      ScratchMemory* scratch);

  static void freeInstance(
      Transaction* ctx,
      const Program* program,
      Instance* instance);

  static void resetInstance(
      Transaction* ctx,
      const Program* program,
      Instance* instance);

  static void mergeInstance(
      Transaction* ctx,
      const Program* program,
      Instance* dst,
      const Instance* src);

  static void saveInstanceState(
      Transaction* ctx,
      const Program* program,
      const Instance* instance,
      OutputStream* os);

  static void loadInstanceState(
      Transaction* ctx,
      const Program* program,
      Instance* instance,
      InputStream* os);

protected:

  static void initProgram(
      Transaction* ctx,
      Program* program,
      Instruction* e);

  static void freeProgram(
      Transaction* ctx,
      const Program* program,
      Instruction* e);

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

