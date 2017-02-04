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

namespace vm {

} // namespace vm

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
    out->setSize(sql_sizeof_static(out->getType()) * vlen);
  }

  for (size_t n = 0; n < vlen; ++n) {
    if (!filter || (*filter)[n]) {
      // evaluate and append to output
      evaluate(ctx, program, entrypoint, stack, instance, argc, argv_cursor);
      popVector(stack, out);
    }

    // increment input iterators
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

//VM::Instance VM::allocInstance(
//    Transaction* ctx,
//    const vm::Program* program,
//    ScratchMemory* scratch) {
//  Instance that;
//
//  that.scratch = scratch->alloc(program->instance_storage_size);
//  initInstance(ctx, program, program->entry_, &that);
//
//  return that;
//}
//
//void VM::freeInstance(
//    Transaction* ctx,
//    const vm::Program* program,
//    Instance* instance) {
//  freeInstance(ctx, program, program->entry_, instance);
//}
//
//void VM::resetInstance(
//    Transaction* ctx,
//    const vm::Program* program,
//    Instance* instance) {
//  resetInstance(ctx, program, program->entry_, instance);
//}
//
//void VM::mergeInstance(
//    Transaction* ctx,
//    const vm::Program* program,
//    Instance* dst,
//    const Instance* src) {
//  mergeInstance(ctx, program, program->entry_, dst, src);
//}
//
//void VM::mergeInstance(
//    Transaction* ctx,
//    const vm::Program* program,
//    vm::Instruction* e,
//    Instance* dst,
//    const Instance* src) {
//  switch (e->type) {
//    case X_CALL_AGGREGATE:
//      e->vtable.merge(
//          Transaction::get(ctx),
//          (char *) dst->scratch + (size_t) e->arg0,
//          (char *) src->scratch + (size_t) e->arg0);
//      break;
//
//    default:
//      break;
//  }
//
//  for (auto cur = e->child; cur != nullptr; cur = cur->next) {
//    mergeInstance(ctx, program, cur, dst, src);
//  }
//}
//
//void VM::initInstance(
//    Transaction* ctx,
//    const vm::Program* program,
//    vm::Instruction* e,
//    Instance* instance) {
//  switch (e->type) {
//    case X_CALL_AGGREGATE:
//      if (e->vtable.init) {
//        e->vtable.init(
//            Transaction::get(ctx),
//            (char *) instance->scratch + (size_t) e->arg0);
//      }
//      break;
//
//    default:
//      break;
//  }
//
//  for (auto cur = e->child; cur != nullptr; cur = cur->next) {
//    initInstance(ctx, program, cur, instance);
//  }
//}
//
//void VM::freeInstance(
//    Transaction* ctx,
//    const vm::Program* program,
//    vm::Instruction* e,
//    Instance* instance) {
//  switch (e->type) {
//    case X_CALL_AGGREGATE:
//      if (e->vtable.free) {
//        e->vtable.free(
//            Transaction::get(ctx),
//            (char *) instance->scratch + (size_t) e->arg0);
//      }
//      break;
//
//    default:
//      break;
//  }
//
//  for (auto cur = e->child; cur != nullptr; cur = cur->next) {
//    freeInstance(ctx, program, cur, instance);
//  }
//}
//
//void VM::resetInstance(
//    Transaction* ctx,
//    const vm::Program* program,
//    vm::Instruction* e,
//    Instance* instance) {
//  switch (e->type) {
//    case X_CALL_AGGREGATE:
//      e->vtable.reset(
//          Transaction::get(ctx),
//          (char *) instance->scratch + (size_t) e->arg0);
//      break;
//
//    default:
//      break;
//  }
//
//  for (auto cur = e->child; cur != nullptr; cur = cur->next) {
//    resetInstance(ctx, program, cur, instance);
//  }
//}
//void VM::saveInstanceState(
//    Transaction* ctx,
//    const vm::Program* program,
//    const Instance* instance,
//    OutputStream* os) {
//  saveInstance(ctx, program, program->entry_, instance, os);
//}
//
//void VM::loadInstanceState(
//    Transaction* ctx,
//    const vm::Program* program,
//    Instance* instance,
//    InputStream* is) {
//  loadInstance(ctx, program, program->entry_, instance, is);
//}
//
//void VM::saveInstance(
//    Transaction* ctx,
//    const vm::Program* program,
//    vm::Instruction* e,
//    const Instance* instance,
//    OutputStream* os) {
//  switch (e->type) {
//    case X_CALL_AGGREGATE:
//      e->vtable.savestate(
//          Transaction::get(ctx),
//          (char *) instance->scratch + (size_t) e->arg0,
//          os);
//      break;
//
//    default:
//      break;
//  }
//
//  for (auto cur = e->child; cur != nullptr; cur = cur->next) {
//    saveInstance(ctx, program, cur, instance, os);
//  }
//}
//
//void VM::loadInstance(
//    Transaction* ctx,
//    const vm::Program* program,
//    vm::Instruction* e,
//    Instance* instance,
//    InputStream* os) {
//  switch (e->type) {
//    case X_CALL_AGGREGATE:
//      e->vtable.loadstate(
//          Transaction::get(ctx),
//          (char *) instance->scratch + (size_t) e->arg0,
//          os);
//      break;
//
//    default:
//      break;
//  }
//
//  for (auto cur = e->child; cur != nullptr; cur = cur->next) {
//    loadInstance(ctx, program, cur, instance, os);
//  }
//}

} // namespace csql

