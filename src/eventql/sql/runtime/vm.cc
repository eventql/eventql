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

VM::Program::Program(
    Transaction* ctx,
    Instruction* entry,
    ScratchMemory&& static_storage,
    size_t dynamic_storage_size,
    SType return_type) :
    ctx_(ctx),
    entry_(entry),
    static_storage_(std::move(static_storage)),
    dynamic_storage_size_(dynamic_storage_size),
    return_type_(return_type),
    has_aggregate_(false) {
  VM::initProgram(ctx_, this, entry_);
}

VM::Program::~Program() {
  VM::freeProgram(ctx_, this, entry_);
}

VM::Instance VM::allocInstance(
    Transaction* ctx,
    const Program* program,
    ScratchMemory* scratch) {
  Instance that;

  if (program->has_aggregate_) {
    that.scratch = scratch->alloc(program->dynamic_storage_size_);
    initInstance(ctx, program, program->entry_, &that);
  } else {
    that.scratch = scratch->construct<SValue>();
  }

  return that;
}

void VM::freeInstance(
    Transaction* ctx,
    const Program* program,
    Instance* instance) {
  if (program->has_aggregate_) {
    freeInstance(ctx, program, program->entry_, instance);
  } else {
    ((SValue*) instance->scratch)->~SValue();
  }
}

void VM::reset(
    Transaction* ctx,
    const Program* program,
    Instance* instance) {
  if (program->has_aggregate_) {
    resetInstance(ctx, program, program->entry_, instance);
  } else {
    *((SValue*) instance->scratch) = SValue();
  }
}

void VM::result(
    Transaction* ctx,
    const Program* program,
    const Instance* instance,
    SValue* out) {
  *out = SValue::newUInt64(0);

  VMRegister out_reg;
  out_reg.data = out->getData();
  out_reg.capacity = out->getCapacity();

  VM::result(ctx, program, instance, &out_reg);
}

void VM::result(
    Transaction* ctx,
    const Program* program,
    const Instance* instance,
    VMRegister* out) {
  assert(program->has_aggregate_);

  return evaluate(
      ctx,
      program,
      const_cast<Instance*>(instance),
      program->entry_,
      0,
      nullptr,
      out);
}

void VM::accumulate(
    Transaction* ctx,
    const Program* program,
    Instance* instance,
    int argc,
    const SValue* argv) {
  void** stackv = nullptr;
  if (argc > 0) {
    stackv = reinterpret_cast<void**>(alloca(sizeof(void*) * argc));
    for (int i = 0; i < argc; ++i) {
      stackv[i] = (void*) argv[i].getData();
    }
  }

  accumulate(ctx, program, instance, argc, stackv);
}

void VM::accumulate(
    Transaction* ctx,
    const Program* program,
    Instance* instance,
    int argc,
    void** argv) {
  assert(program->has_aggregate_);
  accumulate(ctx, program, instance, program->entry_, argc, argv);
}

void VM::evaluate(
    Transaction* ctx,
    const Program* program,
    int argc,
    const SValue* argv,
    SValue* out) {
  void** stackv = nullptr;
  if (argc > 0) {
    stackv = reinterpret_cast<void**>(alloca(sizeof(void*) * argc));
    for (int i = 0; i < argc; ++i) {
      stackv[i] = (void*) argv[i].getData();
    }
  }

  evaluateLegacy(ctx, program, argc, stackv, out);
}

void VM::evaluateLegacy(
    Transaction* ctx,
    const Program* program,
    int argc,
    void** argv,
    SValue* out) {
  assert(out->getType() == program->return_type_);

  VMRegister out_reg;
  out_reg.data = out->getData();
  out_reg.capacity = out->getCapacity();

  evaluate(ctx, program, argc, argv, &out_reg);
}

void VM::evaluate(
    Transaction* ctx,
    const Program* program,
    int argc,
    void** argv,
    VMRegister* out) {
  switch (program->entry_->type) {
    case X_INPUT:
      assert(argv[reinterpret_cast<uint64_t>(program->entry_->arg0)]);
      copyRegister(
          program->return_type_,
          out,
          argv[reinterpret_cast<uint64_t>(program->entry_->arg0)]);
      break;

    case X_LITERAL:
      copyRegister(program->return_type_, out, program->entry_->retval.data);
      break;

    default:
      evaluate(ctx, program, nullptr, program->entry_, argc, argv, out);
      break;
  }
}

void VM::evaluateVector(
    Transaction* ctx,
    const Program* program,
    int argc,
    const SVector* argv,
    size_t vlen,
    SVector* out,
    const std::vector<bool>* filter /* = nullptr */) {
  if (program->entry_->type == X_INPUT && !filter) {
    auto index = reinterpret_cast<uint64_t>(program->entry_->arg0);

    if (index >= argc) {
      RAISE(kRuntimeError, "invalid row index %i", index);
    }

    out->copyFrom(argv + index);
    return;
  }

  void** stackv = nullptr;
  if (argc > 0) {
    stackv = reinterpret_cast<void**>(alloca(sizeof(void*) * argc));
  }

  for (size_t i = 0; i < argc; ++i) {
    if (false) { // FIXME only if input required
      stackv[i] = nullptr;
    } else {
      stackv[i] = (void*) argv[i].getData();
    }
  }

  auto rtype = program->return_type_;
  assert(rtype == out->getType());

  if (sql_sizeof_static(out->getType())) {
    out->increaseCapacity(sql_sizeof_static(out->getType()) * vlen);
    out->setSize(sql_sizeof_static(out->getType()) * vlen);
  }

  VMRegister out_reg;
  out_reg.data = out->getMutableData();
  out_reg.capacity = out->getCapacity();

  for (size_t n = 0; n < vlen; ++n) {
    if (!filter || (*filter)[n]) {
      evaluate(ctx, program, argc, stackv, &out_reg);

      // increment input iterators
      for (size_t i = 0; i < argc; ++i) {
        if (stackv[i]) {
          argv[i].next(&stackv[i]);
        }
      }

      // increment output iterator
      out_reg.capacity -= SVector::next(rtype, &out_reg.data);
    } else {
      // increment input iterators
      for (size_t i = 0; i < argc; ++i) {
        if (stackv[i]) {
          argv[i].next(&stackv[i]);
        }
      }
    }
  }
}

void VM::evaluatePredicateVector(
    Transaction* ctx,
    const Program* program,
    int argc,
    const SVector* argv,
    size_t vlen,
    std::vector<bool>* out,
    size_t* out_cardinality) {
  assert(program->return_type_ == SType::BOOL);

  void** stackv = nullptr;
  if (argc > 0) {
    stackv = reinterpret_cast<void**>(alloca(sizeof(void*) * argc));
  }

  for (size_t i = 0; i < argc; ++i) {
    if (false) { // FIXME only if input required
      stackv[i] = nullptr;
    } else {
      stackv[i] = (void*) argv[i].getData();
    }
  }

  out->resize(vlen);
  *out_cardinality = 0;

  VMRegister out_reg;
  out_reg.data = alloca(1);
  out_reg.capacity = 1;

  auto rtype = program->return_type_;
  for (size_t n = 0; n < vlen; ++n) {
    evaluate(ctx, program, argc, stackv, &out_reg);

    // increment input iterators
    for (size_t i = 0; i < argc; ++i) {
      if (stackv[i]) {
        argv[i].next(&stackv[i]);
      }
    }

    // store result
    uint8_t pred = *static_cast<uint8_t*>(out_reg.data);
    (*out)[n] = pred;
    *out_cardinality += pred;
  }
}

void VM::merge(
    Transaction* ctx,
    const Program* program,
    Instance* dst,
    const Instance* src) {
  if (program->has_aggregate_) {
    mergeInstance(ctx, program, program->entry_, dst, src);
  } else {
    *(SValue*) dst->scratch = *(SValue*) src->scratch;
  }
}

void VM::mergeInstance(
    Transaction* ctx,
    const Program* program,
    Instruction* e,
    Instance* dst,
    const Instance* src) {
  switch (e->type) {
    case X_CALL_AGGREGATE:
      e->vtable.merge(
          Transaction::get(ctx),
          (char *) dst->scratch + (size_t) e->arg0,
          (char *) src->scratch + (size_t) e->arg0);
      break;

    default:
      break;
  }

  for (auto cur = e->child; cur != nullptr; cur = cur->next) {
    mergeInstance(ctx, program, cur, dst, src);
  }
}

void VM::initInstance(
    Transaction* ctx,
    const Program* program,
    Instruction* e,
    Instance* instance) {
  switch (e->type) {
    case X_CALL_AGGREGATE:
      if (e->vtable.init) {
        e->vtable.init(
            Transaction::get(ctx),
            (char *) instance->scratch + (size_t) e->arg0);
      }
      break;

    default:
      break;
  }

  for (auto cur = e->child; cur != nullptr; cur = cur->next) {
    initInstance(ctx, program, cur, instance);
  }
}

void VM::freeInstance(
    Transaction* ctx,
    const Program* program,
    Instruction* e,
    Instance* instance) {
  switch (e->type) {
    case X_CALL_AGGREGATE:
      if (e->vtable.free) {
        e->vtable.free(
            Transaction::get(ctx),
            (char *) instance->scratch + (size_t) e->arg0);
      }
      break;

    default:
      break;
  }

  for (auto cur = e->child; cur != nullptr; cur = cur->next) {
    freeInstance(ctx, program, cur, instance);
  }
}

void VM::resetInstance(
    Transaction* ctx,
    const Program* program,
    Instruction* e,
    Instance* instance) {
  switch (e->type) {
    case X_CALL_AGGREGATE:
      e->vtable.reset(
          Transaction::get(ctx),
          (char *) instance->scratch + (size_t) e->arg0);
      break;

    default:
      break;
  }

  for (auto cur = e->child; cur != nullptr; cur = cur->next) {
    resetInstance(ctx, program, cur, instance);
  }
}

void VM::initProgram(
    Transaction* ctx,
    Program* program,
    Instruction* e) {
  switch (e->type) {
    case X_CALL_AGGREGATE:
      program->has_aggregate_ = true;
      break;

    default:
      break;
  }

  for (auto cur = e->child; cur != nullptr; cur = cur->next) {
    initProgram(ctx, program, cur);
  }
}

void VM::freeProgram(
    Transaction* ctx,
    const Program* program,
    Instruction* e) {
  switch (e->type) {
    case X_LITERAL:
      ((SValue*) e->arg0)->~SValue();
      break;

    case X_REGEX:
      ((RegExp*) e->arg0)->~RegExp();
      break;

    case X_LIKE:
      ((LikePattern*) e->arg0)->~LikePattern();
      break;

    default:
      break;
  }

  for (auto cur = e->child; cur != nullptr; cur = cur->next) {
    freeProgram(ctx, program, cur);
  }
}

void VM::evaluate(
    Transaction* ctx,
    const Program* program,
    Instance* instance,
    Instruction* expr,
    int argc,
    void** argv,
    VMRegister* out) {

  /* execute expression */
  switch (expr->type) {

    //case X_IF: {
    //  SValue cond;
    //  auto cond_expr = expr->child;
    //  evaluate(ctx, program, instance, cond_expr, argc, argv, &cond);

    //  auto branch = cond_expr->next;
    //  if (!cond.getBool()) {
    //    branch = branch->next;
    //  }

    //  evaluate(ctx, program, instance, branch, argc, argv, out);
    //  return;
    //}

    case X_CALL_PURE: {
      void** stackv = nullptr;
      auto stackn = expr->argn;
      if (stackn > 0) {
        stackv = reinterpret_cast<void**>(alloca(sizeof(void*) * expr->argn));

        auto stackp = stackv;
        for (auto cur = expr->child; cur != nullptr; cur = cur->next) {
          switch (cur->type) {
            case X_INPUT:
              *(stackp++) = argv[reinterpret_cast<uint64_t>(cur->arg0)];
              break;

            case X_LITERAL:
              *(stackp++) = cur->retval.data;
              break;

            default:
              evaluate(
                  ctx,
                  program,
                  instance,
                  cur,
                  argc,
                  argv,
                  &cur->retval);

              *(stackp++) = cur->retval.data;
              break;
          }
        }
      }

      expr->vtable.call(Transaction::get(ctx), stackn, stackv, out);
      return;
    }

    case X_CALL_AGGREGATE: {
      assert(instance);
      auto scratch = (char *) instance->scratch + (size_t) expr->arg0;
      expr->vtable.get(Transaction::get(ctx), scratch, out);
      return;
    }

    case X_INPUT:
      assert(argv[reinterpret_cast<uint64_t>(expr->arg0)]);
      copyRegister(
          expr->rettype,
          out,
          argv[reinterpret_cast<uint64_t>(expr->arg0)]);
      break;

    case X_LITERAL:
      copyRegister(expr->rettype, out, expr->retval.data);
      break;

    //case X_REGEX: {
    //  SValue subj;
    //  auto subj_expr = expr->child;
    //  evaluate(ctx, program, instance, subj_expr, argc, argv, &subj);

    //  auto match = ((RegExp*) expr->arg0)->match(subj.getString());
    //  *out = SValue(SValue::BoolType(match));

    //  return;
    //}

    //case X_LIKE: {
    //  SValue subj;
    //  auto subj_expr = expr->child;
    //  evaluate(ctx, program, instance, subj_expr, argc, argv, &subj);

    //  auto match = ((LikePattern*) expr->arg0)->match(subj.getString());
    //  *out = SValue(SValue::BoolType(match));

    //  return;
    //}

  }
}

void VM::accumulate(
    Transaction* ctx,
    const Program* program,
    Instance* instance,
    Instruction* expr,
    int argc,
    void** argv) {

  switch (expr->type) {

    case X_CALL_AGGREGATE: {
      void** stackv = nullptr;
      auto stackn = expr->argn;
      if (stackn > 0) {
        stackv = reinterpret_cast<void**>(alloca(sizeof(void*) * expr->argn));

        auto stackp = stackv;
        for (auto cur = expr->child; cur != nullptr; cur = cur->next) {
          switch (cur->type) {
            case X_INPUT:
              *(stackp++) = argv[reinterpret_cast<uint64_t>(cur->arg0)];
              break;

            case X_LITERAL:
              *(stackp++) = cur->retval.data;
              break;

            default:
              evaluate(
                  ctx,
                  program,
                  instance,
                  cur,
                  argc,
                  argv,
                  &cur->retval);

              *(stackp++) = cur->retval.data;
          }
        }
      }

      auto scratch = (char *) instance->scratch + (size_t) expr->arg0;
      expr->vtable.accumulate(
          Transaction::get(ctx),
          scratch,
          stackn,
          stackv);
      return;
    }

    default: {
      for (auto cur = expr->child; cur != nullptr; cur = cur->next) {
        accumulate(ctx, program, instance, cur, argc, argv);
      }

      return;
    }

  }
}

void VM::saveState(
    Transaction* ctx,
    const Program* program,
    const Instance* instance,
    OutputStream* os) {
  if (program->has_aggregate_) {
    saveInstance(ctx, program, program->entry_, instance, os);
  } else {
    ((SValue*) instance->scratch)->encode(os);
  }
}

void VM::loadState(
    Transaction* ctx,
    const Program* program,
    Instance* instance,
    InputStream* is) {
  if (program->has_aggregate_) {
    loadInstance(ctx, program, program->entry_, instance, is);
  } else {
    ((SValue*) instance->scratch)->decode(is);
  }
}

void VM::saveInstance(
    Transaction* ctx,
    const Program* program,
    Instruction* e,
    const Instance* instance,
    OutputStream* os) {
  switch (e->type) {
    case X_CALL_AGGREGATE:
      e->vtable.savestate(
          Transaction::get(ctx),
          (char *) instance->scratch + (size_t) e->arg0,
          os);
      break;

    default:
      break;
  }

  for (auto cur = e->child; cur != nullptr; cur = cur->next) {
    saveInstance(ctx, program, cur, instance, os);
  }
}

void VM::loadInstance(
    Transaction* ctx,
    const Program* program,
    Instruction* e,
    Instance* instance,
    InputStream* os) {
  switch (e->type) {
    case X_CALL_AGGREGATE:
      e->vtable.loadstate(
          Transaction::get(ctx),
          (char *) instance->scratch + (size_t) e->arg0,
          os);
      break;

    default:
      break;
  }

  for (auto cur = e->child; cur != nullptr; cur = cur->next) {
    loadInstance(ctx, program, cur, instance, os);
  }
}

void VM::copyRegister(SType type, VMRegister* dst, void* src) {
  assert(dst->data);
  assert(src);

  switch (type) { // FIXME
    case SType::UINT64:
    case SType::TIMESTAMP64:
      memcpy(dst->data, src, sizeof(uint64_t));
      break;

    case SType::INT64:
      memcpy(dst->data, src, sizeof(int64_t));
      break;

    case SType::FLOAT64:
      memcpy(dst->data, src, sizeof(double));
      break;
  }
}

}
