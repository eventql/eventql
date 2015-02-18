// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/flow/vm/Program.h>
#include <xzero/flow/vm/ConstantPool.h>
#include <xzero/flow/vm/Handler.h>
#include <xzero/flow/vm/Instruction.h>
#include <xzero/flow/vm/Runtime.h>
#include <xzero/flow/vm/Runner.h>
#include <xzero/flow/vm/Match.h>
#include <utility>
#include <vector>
#include <memory>
#include <new>

namespace xzero {
namespace flow {
namespace vm {

/* {{{ possible binary file format
 * ----------------------------------------------
 * u32                  magic number (0xbeafbabe)
 * u32                  version
 * u64                  flags
 * u64                  register count
 * u64                  code start
 * u64                  code size
 * u64                  integer const-table start
 * u64                  integer const-table element count
 * u64                  string const-table start
 * u64                  string const-table element count
 * u64                  regex const-table start (stored as string)
 * u64                  regex const-table element count
 * u64                  debug source-lines-table start
 * u64                  debug source-lines-table element count
 *
 * u32[]                code segment
 * u64[]                integer const-table segment
 * u64[]                string const-table segment
 * {u32, u8[]}[]        strings
 * {u32, u32, u32}[]    debug source lines segment
 */  // }}}

Program::Program(ConstantPool&& cp)
    : cp_(std::move(cp)),
      runtime_(nullptr),
      handlers_(),
      matches_(),
      nativeHandlers_(),
      nativeFunctions_() {
  for (const auto& handler : cp_.getHandlers())
    createHandler(handler.first, handler.second);

  setup(cp_.getMatchDefs());
}

Program::~Program() {
  for (auto m : matches_) delete m;

  for (auto& handler : handlers_) delete handler;
}

void Program::setup(const std::vector<MatchDef>& matches) {
  for (size_t i = 0, e = matches.size(); i != e; ++i) {
    const MatchDef& def = matches[i];
    switch (def.op) {
      case MatchClass::Same:
        matches_.push_back(new MatchSame(def, this));
        break;
      case MatchClass::Head:
        matches_.push_back(new MatchHead(def, this));
        break;
      case MatchClass::Tail:
        matches_.push_back(new MatchTail(def, this));
        break;
      case MatchClass::RegExp:
        matches_.push_back(new MatchRegEx(def, this));
        break;
    }
  }
}

Handler* Program::createHandler(const std::string& name) {
  Handler* handler = new Handler(this, name, {});
  handlers_.push_back(handler);
  return handler;
}

Handler* Program::createHandler(const std::string& name,
                                const std::vector<Instruction>& instructions) {
  Handler* handler = new Handler(this, name, instructions);
  handlers_.push_back(handler);

  return handler;
}

Handler* Program::findHandler(const std::string& name) const {
  for (auto handler : handlers_)
    if (handler->name() == name) return handler;

  return nullptr;
}

int Program::indexOf(const Handler* handler) const {
  for (int i = 0, e = handlers_.size(); i != e; ++i)
    if (handlers_[i] == handler) return i;

  return -1;
}

void Program::dump() {
  printf("; Program\n");

  cp_.dump();

  for (size_t i = 0, e = handlers_.size(); i != e; ++i) {
    Handler* handler = handlers_[i];
    printf("\n.handler %-20s ; #%zu (%zu registers, %zu instructions)\n",
           handler->name().c_str(), i,
           handler->registerCount() ? handler->registerCount() - 1
                                    : 0,  // r0 is never used
           handler->code().size());
    handler->disassemble();
  }

  printf("\n\n");
}

/**
 * Maps all native functions/handlers to their implementations (report
 *unresolved symbols)
 *
 * \param runtime the runtime to link this program against, resolving any
 *external native symbols.
 * \retval true Linking succeed.
 * \retval false Linking failed due to unresolved native signatures not found in
 *the runtime.
 */
bool Program::link(Runtime* runtime) {
  runtime_ = runtime;
  int errors = 0;

  // load runtime modules
  for (const auto& module : cp_.getModules()) {
    if (!runtime->import(module.first, module.second, nullptr)) {
      errors++;
    }
  }

  // link nattive handlers
  nativeHandlers_.resize(cp_.getNativeHandlerSignatures().size());
  size_t i = 0;
  for (const auto& signature : cp_.getNativeHandlerSignatures()) {
    // map to nativeHandlers_[i]
    if (NativeCallback* cb = runtime->find(signature)) {
      nativeHandlers_[i] = cb;
    } else {
      nativeHandlers_[i] = nullptr;
      fprintf(stderr, "Unresolved native handler signature: %s\n",
              signature.c_str());
      // TODO unresolvedSymbols_.push_back(signature);
      errors++;
    }
    ++i;
  }

  // link nattive functions
  nativeFunctions_.resize(cp_.getNativeFunctionSignatures().size());
  i = 0;
  for (const auto& signature : cp_.getNativeFunctionSignatures()) {
    if (NativeCallback* cb = runtime->find(signature)) {
      nativeFunctions_[i] = cb;
    } else {
      nativeFunctions_[i] = nullptr;
      fprintf(stderr, "Unresolved native function signature: %s\n",
              signature.c_str());
      // TODO unresolvedSymbols_.push_back(signature);
      errors++;
    }
    ++i;
  }

  return errors == 0;
}

}  // namespace vm
}  // namespace flow
}  // namespace xzero
