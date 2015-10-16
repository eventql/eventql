/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/inspect.h"
#include "zbase/JavaScriptContext.h"

using namespace stx;

namespace zbase {

static JSClass global_class = { "global", JSCLASS_GLOBAL_FLAGS };

JavaScriptContext::JavaScriptContext() {
  runtime_ = JS_NewRuntime(8 * 1024 * 1024);
  if (!runtime_) {
    RAISE(kRuntimeError, "error while initializing JavaScript runtime");
  }

  ctx_ = JS_NewContext(runtime_, 8192);
  if (!ctx_) {
    RAISE(kRuntimeError, "error while initializing JavaScript context");
  }

  JS_BeginRequest(ctx_);

  global_ = JS_NewGlobalObject(
      ctx_,
      &global_class,
      nullptr,
      JS::FireOnNewGlobalHook);

  if (!global_) {
    JS_EndRequest(ctx_);
    RAISE(kRuntimeError, "error while initializing JavaScript context");
  }

  {
    JSAutoCompartment ac(ctx_, global_);
    JS_InitStandardClasses(ctx_, global_);
  }

  JS_EndRequest(ctx_);
}

JavaScriptContext::~JavaScriptContext() {
  JS_DestroyContext(ctx_);
  JS_DestroyRuntime(runtime_);
}

void JavaScriptContext::loadProgram(const String& program) {
  JSAutoRequest js_req(ctx_);
  JSAutoCompartment ac(ctx_, global_);

  JS::RootedValue rval(ctx_);

  JS::CompileOptions opts(ctx_);
  opts.setFileAndLine("<mapreduce>", 1);

  if (!JS::Evaluate(
        ctx_,
        global_,
        opts,
        program.c_str(),
        program.size(),
        &rval)) {
    JS_EndRequest(ctx_);
    RAISE(kRuntimeError, "JavaScript execution failed");
  }
}

void JavaScriptContext::callMapFunction(
    const String& method_name,
    const String& json_string) {
  auto json_wstring = StringUtil::convertUTF8To16(json_string);

  JSAutoRequest js_req(ctx_);
  JSAutoCompartment js_comp(ctx_, global_);

  JS::RootedValue json(ctx_);
  if (!JS_ParseJSON(ctx_, json_wstring.c_str(), json_wstring.size(), &json)) {
    RAISE(kRuntimeError, "invalid JSON");
  }

  JS::AutoValueArray<1> argv(ctx_);
  argv[0].set(json);

  JS::RootedValue rval(ctx_);
  JS_CallFunctionName(ctx_, global_, method_name.c_str(), argv, &rval);

  String str = JS_EncodeString(ctx_, rval.toString());
  stx::iputs("call $0 with $1 -> result: $2", method_name, json_string, str);
}

} // namespace zbase

