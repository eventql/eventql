// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/flow/vm/Handler.h>
#include <xzero/flow/vm/Runner.h>
#include <xzero/flow/vm/Instruction.h>
#include <xzero/sysconfig.h>

namespace xzero {
namespace flow {
namespace vm {

Handler::Handler() {
}

Handler::Handler(Program* program, const std::string& name,
                 const std::vector<Instruction>& code)
    : program_(program),
      name_(name),
      registerCount_(computeRegisterCount(code.data(), code.size())),
      code_(code)
#if defined(ENABLE_FLOW_DIRECT_THREADED_VM)
      ,
      directThreadedCode_()
#endif
{
}

Handler::Handler(const Handler& v)
    : program_(v.program_),
      name_(v.name_),
      registerCount_(v.registerCount_),
      code_(v.code_)
#if defined(ENABLE_FLOW_DIRECT_THREADED_VM)
      ,
      directThreadedCode_(v.directThreadedCode_)
#endif
{
}

Handler::Handler(Handler&& v)
    : program_(std::move(v.program_)),
      name_(std::move(v.name_)),
      registerCount_(std::move(v.registerCount_)),
      code_(std::move(v.code_))
#if defined(ENABLE_FLOW_DIRECT_THREADED_VM)
      ,
      directThreadedCode_(std::move(v.directThreadedCode_))
#endif
{
}

Handler::~Handler() {}

void Handler::setCode(const std::vector<Instruction>& code) {
  code_ = code;
  registerCount_ = computeRegisterCount(code_.data(), code_.size());
}

void Handler::setCode(std::vector<Instruction>&& code) {
  code_ = std::move(code);
  registerCount_ = computeRegisterCount(code_.data(), code_.size());

#if defined(ENABLE_FLOW_DIRECT_THREADED_VM)
  directThreadedCode_.clear();
#endif
}

std::unique_ptr<Runner> Handler::createRunner() { return Runner::create(this); }

bool Handler::run(void* userdata) {
  auto runner = createRunner();
  runner->setUserData(userdata);
  return runner->run();
}

void Handler::disassemble() {
  printf("%s", vm::disassemble(code_.data(), code_.size()).c_str());
}

}  // namespace vm
}  // namespace flow
}  // namespace xzero
