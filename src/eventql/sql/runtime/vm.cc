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
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <eventql/sql/runtime/compiler.h>
#include <eventql/sql/runtime/LikePattern.h>
#include <eventql/sql/svalue.h>
#include <eventql/sql/runtime/vm.h>
#include <eventql/util/exception.h>
#include <eventql/util/RegExp.h>

namespace csql {

VMStack::VMStack() :
  data(nullptr),
  top(nullptr),
  limit(nullptr) {}

namespace vm {

Instruction::Instruction(
    InstructionType _type,
    intptr_t _arg0) :
    type(_type),
    arg0(_arg0),
    argt(SType::NIL) {}

Instruction::Instruction(
    InstructionType _type,
    intptr_t _arg0,
    SType _argt) :
    type(_type),
    arg0(_arg0),
    argt(_argt) {}

EntryPoint::EntryPoint() : offset(0) {}
EntryPoint::EntryPoint(size_t _offset) : offset(_offset) {}

Program::Program() :
    instance_storage_size(0),
    return_type(SType::NIL),
    instance_reset(nullptr),
    instance_init(nullptr),
    instance_free(nullptr),
    instance_merge(nullptr),
    instance_savestate(nullptr),
    instance_loadstate(nullptr) {}

void growStack(VMStack* stack, size_t bytes) {
  auto grow_bytes =
      (bytes + (kStackBlockSize - 1) / kStackBlockSize) * kStackBlockSize;

  if (stack->data) {
    auto old_size = stack->limit - stack->data;
    auto old_top = stack->limit - stack->top;
    auto new_size = old_size + grow_bytes;

    stack->data = (char*) realloc(stack->data, new_size);
    stack->limit = stack->data + new_size;
    stack->top = stack->limit - old_top;
  } else {
    stack->data = (char*) malloc(grow_bytes);
    stack->limit = stack->data + grow_bytes;
    stack->top = stack->limit;
  }
}

} // namespace vm


using PureFunctionPtr = void (*)(sql_txn*, VMStack*);
using InstanceFunctionPtr = void (*)(sql_txn*, void*, VMStack*);

void VM::evaluate(
    Transaction* ctx,
    const vm::Program* program,
    vm::EntryPoint entrypoint,
    VMStack* stack,
    Instance* instance,
    int argc,
    void** argv) {
  auto instructions = program->instructions.data();

  for (size_t pc = entrypoint.offset; ;) {
    assert(pc < program->instructions.size());

    const auto& op = instructions[pc];
    switch (op.type) {

      case vm::X_CALL_PURE:
        ((PureFunctionPtr) op.arg0)(Transaction::get(ctx), stack);
        ++pc;
        continue;

      case vm::X_CALL_INSTANCE:
        ((InstanceFunctionPtr) op.arg0)(Transaction::get(ctx), instance, stack);
        ++pc;
        continue;

      case vm::X_LITERAL:
        pushUnboxed(stack, op.argt, reinterpret_cast<const void*>(op.arg0));
        ++pc;
        continue;

      case vm::X_INPUT:
        pushUnboxed(stack, op.argt, argv[op.arg0]);
        ++pc;
        continue;

      case vm::X_JUMP:
        pc = op.arg0;
        continue;

      case vm::X_CJUMP:
        pc = popBool(stack) ? op.arg0 : pc + 1;
        continue;

      case vm::X_RETURN:
        return;

    }
  }
}

void VM::evaluateBoxed(
    Transaction* ctx,
    const vm::Program* program,
    vm::EntryPoint entrypoint,
    VMStack* stack,
    Instance* instance,
    int argc,
    const SValue* argv) {
  void** argv_unboxed = nullptr;
  if (argc > 0) {
    argv_unboxed = reinterpret_cast<void**>(alloca(sizeof(void*) * argc));
    for (int i = 0; i < argc; ++i) {
      argv_unboxed[i] = (void*) argv[i].getData();
    }
  }

  evaluate(ctx, program, entrypoint, stack, instance, argc, argv_unboxed);
}

void VM::evaluateVector(
    Transaction* ctx,
    const vm::Program* program,
    vm::EntryPoint entrypoint,
    VMStack* stack,
    Instance* instance,
    int argc,
    const SVector* argv,
    size_t vlen,
    SVector* out,
    const std::vector<bool>* filter /* = nullptr */) {
  //if (program->entry_->type == X_INPUT && !filter) {
  //  auto index = reinterpret_cast<uint64_t>(program->entry_->arg0);

  //  if (index >= argc) {
  //    RAISE(kRuntimeError, "invalid row index %i", index);
  //  }

  //  out->copyFrom(argv + index);
  //  return;
  //}

  void** argv_cursor = nullptr;
  if (argc > 0) {
    argv_cursor = reinterpret_cast<void**>(alloca(sizeof(void*) * argc));
  }

  for (size_t i = 0; i < argc; ++i) {
    argv_cursor[i] = (void*) argv[i].getData();
  }

  auto rtype = program->return_type;
  assert(rtype == out->getType());

  if (sql_sizeof_static(out->getType())) {
    out->increaseCapacity(sql_sizeof_static(out->getType()) * vlen);
  }

  for (size_t n = 0; n < vlen; ++n) {
    if (!filter || (*filter)[n]) {
      evaluate(ctx, program, entrypoint, stack, instance, argc, argv_cursor);
      popVector(stack, out);
    }

    for (size_t i = 0; i < argc; ++i) {
      if (argv_cursor[i]) {
        argv[i].next(&argv_cursor[i]);
      }
    }
  }
}

void VM::evaluatePredicateVector(
    Transaction* ctx,
    const vm::Program* program,
    vm::EntryPoint entrypoint,
    VMStack* stack,
    Instance* instance,
    int argc,
    const SVector* argv,
    size_t vlen,
    std::vector<bool>* out,
    size_t* out_cardinality) {
  assert(program->return_type == SType::BOOL);

  void** argv_cursor = nullptr;
  if (argc > 0) {
    argv_cursor = reinterpret_cast<void**>(alloca(sizeof(void*) * argc));
  }

  for (size_t i = 0; i < argc; ++i) {
    argv_cursor[i] = (void*) argv[i].getData();
  }

  out->resize(vlen);
  *out_cardinality = 0;

  auto rtype = program->return_type;
  for (size_t n = 0; n < vlen; ++n) {
    evaluate(ctx, program, entrypoint, stack, instance, argc, argv_cursor);

    // store result
    uint8_t pred = popBool(stack);
    (*out)[n] = pred;
    *out_cardinality += pred;

    // increment input iterators
    for (size_t i = 0; i < argc; ++i) {
      if (argv_cursor[i]) {
        argv[i].next(&argv_cursor[i]);
      }
    }
  }
}

VM::Instance VM::allocInstance(
    Transaction* ctx,
    const vm::Program* program,
    ScratchMemory* scratch) {
  Instance self;
  self = scratch->alloc(program->instance_storage_size);

  if (program->instance_init) {
    program->instance_init(Transaction::get(ctx), self);
  }

  return self;
}

void VM::freeInstance(
    Transaction* ctx,
    const vm::Program* program,
    Instance* instance) {
  if (program->instance_free) {
    program->instance_free(Transaction::get(ctx), instance);
  }
}

void VM::resetInstance(
    Transaction* ctx,
    const vm::Program* program,
    Instance* instance) {
  program->instance_reset(Transaction::get(ctx), instance);
}

void VM::mergeInstance(
    Transaction* ctx,
    const vm::Program* program,
    Instance* dst,
    const Instance* src) {
  program->instance_merge(Transaction::get(ctx), dst, src);
}

void VM::saveInstanceState(
    Transaction* ctx,
    const vm::Program* program,
    const Instance* instance,
    OutputStream* os) {
  program->instance_savestate(Transaction::get(ctx), instance, os);
}

void VM::loadInstanceState(
    Transaction* ctx,
    const vm::Program* program,
    Instance* instance,
    InputStream* is) {
  program->instance_loadstate(Transaction::get(ctx), instance, is);
}

} // namespace csql

