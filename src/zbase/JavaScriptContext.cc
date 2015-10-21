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
#include "js/Conversions.h"

using namespace stx;

namespace zbase {

static JSClass global_class = { "global", JSCLASS_GLOBAL_FLAGS };

static bool write_json_to_buf(const char16_t* str, uint32_t strlen, void* out) {
  *static_cast<String*>(out) += StringUtil::convertUTF16To8(
      std::basic_string<char16_t>(str, strlen));

  return true;
}

JavaScriptContext::JavaScriptContext() {
  runtime_ = JS_NewRuntime(8 * 1024 * 1024);
  if (!runtime_) {
    RAISE(kRuntimeError, "error while initializing JavaScript runtime");
  }

  ctx_ = JS_NewContext(runtime_, 8192);
  if (!ctx_) {
    RAISE(kRuntimeError, "error while initializing JavaScript context");
  }

  JSAutoRequest js_req(ctx_);

  global_ = JS_NewGlobalObject(
      ctx_,
      &global_class,
      nullptr,
      JS::FireOnNewGlobalHook);

  if (!global_) {
    RAISE(kRuntimeError, "error while initializing JavaScript context");
  }

  {
    JSAutoCompartment ac(ctx_, global_);
    JS_InitStandardClasses(ctx_, global_);
  }
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
    RAISE(kRuntimeError, "JavaScript execution failed");
  }
}

void JavaScriptContext::callMapFunction(
    const String& method_name,
    const String& json_string,
    Vector<Pair<String, String>>* tuples) {
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
  if (!JS_CallFunctionName(ctx_, global_, method_name.c_str(), argv, &rval)) {
    RAISE(kRuntimeError, "map function failed");
  }

  enumerateTuples(&rval, tuples);
}

void JavaScriptContext::callReduceFunction(
    const String& method_name,
    const String& key,
    const Vector<String>& values,
    Vector<Pair<String, String>>* tuples) {
  JSAutoRequest js_req(ctx_);
  JSAutoCompartment js_comp(ctx_, global_);

  auto val_array_ptr = JS_NewArrayObject(ctx_, values.size());
  if (!val_array_ptr) {
    RAISE(kRuntimeError, "reduce function execution error: out of memory");
  }

  JS::RootedObject val_array(ctx_, val_array_ptr);
  for (size_t i = 0; i < values.size(); ++i) {
    auto val_str_ptr = JS_NewStringCopyN(
        ctx_,
        values[i].data(),
        values[i].size());
    if (!val_str_ptr) {
      RAISE(kRuntimeError, "reduce function execution error: out of memory");
    }

    JS::RootedString val_str(ctx_, val_str_ptr);
    if (!JS_SetElement(ctx_, val_array, i, val_str)) {
      RAISE(kRuntimeError, "reduce function execution error: out of memory");
    }
  }

  auto key_str_ptr = JS_NewStringCopyN(ctx_, key.data(), key.size());
  if (!key_str_ptr) {
    RAISE(kRuntimeError, "reduce function execution error: out of memory");
  }

  JS::AutoValueArray<2> argv(ctx_);
  argv[0].setString(key_str_ptr);
  argv[1].setObject(*val_array);

  JS::RootedValue rval(ctx_);
  if (!JS_CallFunctionName(ctx_, global_, method_name.c_str(), argv, &rval)) {
    RAISE(kRuntimeError, "reduce function failed");
  }

  enumerateTuples(&rval, tuples);
}

void JavaScriptContext::enumerateTuples(
    JS::RootedValue* src,
    Vector<Pair<String, String>>* dst) const {
  if (!src->isObject()) {
    RAISE(kRuntimeError, "reduce function must return a list/array of tuples");
  }

  JS::RootedObject list(ctx_, &src->toObject());
  JS::AutoIdArray list_enum(ctx_, JS_Enumerate(ctx_, list));
  for (size_t i = 0; i < list_enum.length(); ++i) {
    JS::RootedValue elem(ctx_);
    JS::RootedValue elem_key(ctx_);
    JS::RootedValue elem_value(ctx_);
    JS::Rooted<jsid> elem_id(ctx_, list_enum[i]);
    if (!JS_GetPropertyById(ctx_, list, elem_id, &elem)) {
      RAISE(kIllegalStateError);
    }

    if (!elem.isObject()) {
      RAISE(kRuntimeError, "reduce function must return a list/array of tuples");
    }

    JS::RootedObject elem_obj(ctx_, &elem.toObject());

    if (!JS_GetProperty(ctx_, elem_obj, "0", &elem_key)) {
      RAISE(kRuntimeError, "reduce function must return a list/array of tuples");
    }

    if (!JS_GetProperty(ctx_, elem_obj, "1", &elem_value)) {
      RAISE(kRuntimeError, "reduce function must return a list/array of tuples");
    }

    auto tkey_jstr = JS::ToString(ctx_, elem_key);
    if (!tkey_jstr) {
      RAISE(kRuntimeError, "first tuple element must be a string");
    }

    auto tkey_cstr = JS_EncodeString(ctx_, tkey_jstr);
    String tkey(tkey_cstr);
    JS_free(ctx_, tkey_cstr);

    String tval;
    JS::RootedObject replacer(ctx_);
    JS::RootedValue space(ctx_);
    if (!JS_Stringify(
            ctx_,
            &elem_value,
            replacer,
            space,
            &write_json_to_buf,
            &tval)) {
      RAISE(kRuntimeError, "second tuple element must be convertible to JSON");
    }

    dst->emplace_back(tkey, tval);
  }
}

Option<String> JavaScriptContext::getMapReduceJobJSON() {
  JSAutoRequest js_req(ctx_);
  JSAutoCompartment js_comp(ctx_, global_);

  JS::RootedValue job_def(ctx_);
  if (!JS_GetProperty(ctx_, global_, "__z1_mr_job", &job_def)) {
    return None<String>();
  }

  String json_str;
  JS::RootedObject replacer(ctx_);
  JS::RootedValue space(ctx_);
  if (!JS_Stringify(
          ctx_,
          &job_def,
          replacer,
          space,
          &write_json_to_buf,
          &json_str)) {
    RAISE(kRuntimeError, "illegal job definition");
  }

  return Some(json_str);
}

} // namespace zbase

