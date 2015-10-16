/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "zbase/JavaScriptContext.h"

using namespace stx;

namespace zbase {

static JSClass global_class = { "global", JSCLASS_GLOBAL_FLAGS };

JavaScriptContext::JavaScriptContext(JSRuntime* runtime) {
  ctx_ = JS_NewContext(runtime, 8192);
  if (!ctx_) {
    RAISE(kRuntimeError, "error while initializing JavaScript context");
  }

  global_ = JS_NewGlobalObject(
      ctx_,
      &global_class,
      nullptr,
      JS::FireOnNewGlobalHook);

  if (!global_) {
    RAISE(kRuntimeError, "error while initializing JavaScript context");
  }
}


JavaScriptContext::~JavaScriptContext() {
  JS_DestroyContext(ctx_);
}

} // namespace zbase

