// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/flow/vm/Match.h>
#include <xzero/flow/vm/Program.h>
#include <xzero/flow/vm/Handler.h>
#include <xzero/flow/vm/Runner.h>

namespace xzero {
namespace flow {
namespace vm {

// {{{ Match
Match::Match(const MatchDef& def, Program* program)
    : def_(def), program_(program), handler_(program->handler(def.handlerId)) {}

Match::~Match() {}
// }}} Match
// {{{ MatchSame
MatchSame::MatchSame(const MatchDef& def, Program* program)
    : Match(def, program), map_() {
  for (const auto& one : def.cases) {
    map_[program->constants().getString(one.label)] = one.pc;
  }
}

MatchSame::~MatchSame() {}

uint64_t MatchSame::evaluate(const FlowString* condition, Runner* env) const {
  const auto i = map_.find(*condition);
  if (i != map_.end()) return i->second;

  return def_.elsePC;  // no match found
}
// }}}
// {{{ MatchHead
MatchHead::MatchHead(const MatchDef& def, Program* program)
    : Match(def, program), map_() {
  for (const auto& one : def.cases) {
    map_.insert(program->constants().getString(one.label), one.pc);
  }
}

MatchHead::~MatchHead() {}

uint64_t MatchHead::evaluate(const FlowString* condition, Runner* env) const {
  uint64_t result;
  if (map_.lookup(*condition, &result)) return result;

  return def_.elsePC;  // no match found
}
// }}}
// {{{ MatchTail
MatchTail::MatchTail(const MatchDef& def, Program* program)
    : Match(def, program), map_() {
  for (const auto& one : def.cases) {
    map_.insert(program->constants().getString(one.label), one.pc);
  }
}

MatchTail::~MatchTail() {}

uint64_t MatchTail::evaluate(const FlowString* condition, Runner* env) const {
  uint64_t result;
  if (map_.lookup(*condition, &result)) return result;

  return def_.elsePC;  // no match found
}
// }}}
// {{{ MatchRegEx
MatchRegEx::MatchRegEx(const MatchDef& def, Program* program)
    : Match(def, program), map_() {
  for (const auto& one : def.cases) {
    map_.push_back(
        std::make_pair(&program->constants().getRegExp(one.label), one.pc));
  }
}

MatchRegEx::~MatchRegEx() {}

uint64_t MatchRegEx::evaluate(const FlowString* condition, Runner* env) const {
  RegExpContext* cx = (RegExpContext*)env->userdata();
  RegExp::Result* rs = cx ? cx->regexMatch() : nullptr;
  for (const auto& one : map_) {
    if (one.first->match(*condition, rs)) {
      return one.second;
    }
  }

  return def_.elsePC;  // no match found
}
// }}}

}  // namespace vm
}  // namespace flow
}  // namespace xzero
