/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
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
#include "eventql/util/inspect.h"
#include "eventql/util/assets.h"
#include "eventql/JavaScriptContext.h"
#include "js/Conversions.h"
#include "jsapi.h"
#include "jsstr.h"
#include <iostream>
#include <locale>
#include <string>
#include <codecvt>

using namespace stx;

namespace eventql {

JSClass JavaScriptContext::kGlobalJSClass = { "global", JSCLASS_GLOBAL_FLAGS };

static bool write_json_to_buf(const char16_t* str, uint32_t strlen, void* out) {
  auto outstr = static_cast<String*>(out);
  outstr->reserve(outstr->size() + strlen);

  std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf8to16;
  *outstr += utf8to16.to_bytes(str, str + strlen);

  return true;
}

JavaScriptContext::JavaScriptContext(
    const String& customer,
    RefPtr<MapReduceJobSpec> job,
    TSDBService* tsdb,
    RefPtr<MapReduceTaskBuilder> task_builder,
    RefPtr<MapReduceScheduler> scheduler,
    size_t memlimit /* = kDefaultMemLimit */) :
    customer_(customer),
    job_(job),
    tsdb_(tsdb),
    task_builder_(task_builder),
    scheduler_(scheduler) {
  {
    runtime_ = JS_NewRuntime(memlimit);
    if (!runtime_) {
      RAISE(kRuntimeError, "error while initializing JavaScript runtime");
    }

    JS::RuntimeOptionsRef(runtime_)
        .setBaseline(true)
        .setIon(true)
        .setAsmJS(true);

    JS_SetRuntimePrivate(runtime_, this);
    JS_SetErrorReporter(runtime_, &JavaScriptContext::dispatchError);

    ctx_ = JS_NewContext(runtime_, 8192);
    if (!ctx_) {
      RAISE(kRuntimeError, "error while initializing JavaScript context");
    }

    JSAutoRequest js_req(ctx_);

    global_ = JS_NewGlobalObject(
        ctx_,
        &kGlobalJSClass,
        nullptr,
        JS::FireOnNewGlobalHook);

    if (!global_) {
      RAISE(kRuntimeError, "error while initializing JavaScript context");
    }

    {
      JSAutoCompartment ac(ctx_, global_);
      JS_InitStandardClasses(ctx_, global_);

      JS_DefineFunction(
          ctx_,
          global_,
          "z1_log",
          &JavaScriptContext::dispatchLog,
          0,
          0);

      JS_DefineFunction(
          ctx_,
          global_,
          "z1_returnresult",
          &JavaScriptContext::returnResult,
          0,
          0);

      JS_DefineFunction(
          ctx_,
          global_,
          "z1_listpartitions",
          &JavaScriptContext::listPartitions,
          0,
          0);

      JS_DefineFunction(
          ctx_,
          global_,
          "z1_executemr",
          &JavaScriptContext::executeMapReduce,
          0,
          0);
    }
  }

  loadProgram(Assets::getAsset("eventql/mapreduce/prelude.js"));
}

JavaScriptContext::~JavaScriptContext() {
  JS_DestroyContext(ctx_);
  JS_DestroyRuntime(runtime_);
}

void JavaScriptContext::storeError(
    const String& error,
    size_t line /* = 0 */,
    size_t column /* = 0 */) {
  current_error_ = error;
  current_error_line_ = line;
  current_error_column_ = column;
}

void JavaScriptContext::dispatchError(
    JSContext* ctx,
    const char* message,
    JSErrorReport* report) {
  auto rt = JS_GetRuntime(ctx);
  auto rt_userdata = JS_GetRuntimePrivate(rt);
  if (rt_userdata) {
    auto req = static_cast<JavaScriptContext*>(rt_userdata);
    req->storeError(message, report->lineno, report->column);
  }
}

bool JavaScriptContext::dispatchLog(
    JSContext* ctx,
    unsigned argc,
    JS::Value* vp) {
  auto args = JS::CallArgsFromVp(argc, vp);
  if (args.length() < 1 || !args[0].isString()) {
    return false;
  }

  auto rt = JS_GetRuntime(ctx);
  auto rt_userdata = JS_GetRuntimePrivate(rt);
  if (rt_userdata) {
    JS::RootedString log_rstr(ctx, args[0].toString());
    auto log_cstr = JS_EncodeStringToUTF8(ctx, log_rstr);
    String log_str(log_cstr);
    JS_free(ctx, log_cstr);

    auto self = static_cast<JavaScriptContext*>(rt_userdata);
    if (self->job_.get()) {
      try {
        self->job_->sendLogline(log_str);
      } catch (const StandardException& e) {
        self->storeError(e.what());
        return false;
      }
    }
  }

  args.rval().set(JSVAL_TRUE);
  return true;
}

bool JavaScriptContext::returnResult(
    JSContext* ctx,
    unsigned argc,
    JS::Value* vp) {
  auto args = JS::CallArgsFromVp(argc, vp);
  if (args.length() < 1 || !args[0].isString()) {
    return false;
  }

  auto rt = JS_GetRuntime(ctx);
  auto rt_userdata = JS_GetRuntimePrivate(rt);
  if (rt_userdata) {
    JS::RootedString result_rstr(ctx, args[0].toString());
    auto result_cstr = JS_EncodeStringToUTF8(ctx, result_rstr);
    String result_str(result_cstr);
    JS_free(ctx, result_cstr);

    auto self = static_cast<JavaScriptContext*>(rt_userdata);
    if (self->job_.get()) {
      try {
        self->job_->sendResult(result_str);
      } catch (const StandardException& e) {
        self->storeError(e.what());
        return false;
      }
    }
  }

  args.rval().set(JSVAL_TRUE);
  return true;
}


bool JavaScriptContext::listPartitions(
    JSContext* ctx,
    unsigned argc,
    JS::Value* vp) {
  auto args = JS::CallArgsFromVp(argc, vp);
  if (args.length() != 3 ||
      !args[0].isString() ||
      !args[1].isString() ||
      !args[2].isString()) {
    return false;
  }

  auto rt = JS_GetRuntime(ctx);
  auto self = (JavaScriptContext*) JS_GetRuntimePrivate(rt);
  if (!self) {
    return false;
  }

  auto table_name_cstr = JS_EncodeString(ctx, args[0].toString());
  String table_name(table_name_cstr);
  JS_free(ctx, table_name_cstr);

  auto from_cstr = JS_EncodeString(ctx, args[1].toString());
  String from(from_cstr);
  JS_free(ctx, from_cstr);

  auto until_cstr = JS_EncodeString(ctx, args[2].toString());
  String until(until_cstr);
  JS_free(ctx, until_cstr);

  Vector<TimeseriesPartition> partitions;
  try {
    partitions = self->tsdb_->listPartitions(
        self->customer_,
        table_name,
        std::stoull(from),
        std::stoull(until));
  } catch (const StandardException& e) {
    self->storeError(e.what());
    return false;
  }

  auto part_array_ptr = JS_NewArrayObject(ctx, partitions.size());
  if (!part_array_ptr) {
    RAISE(kRuntimeError, "JavaScript execution error: out of memory");
  }

  JS::RootedObject part_array(ctx, part_array_ptr);
  for (size_t i = 0; i < partitions.size(); ++i) {
    auto part_obj_ptr = JS_NewObject(ctx, NULL);
    if (!part_obj_ptr) {
      RAISE(kRuntimeError, "reduce function execution error: out of memory");
    }

    JS::RootedObject part_obj(ctx, part_obj_ptr);
    if (!JS_SetElement(ctx, part_array, i, part_obj)) {
      RAISE(kRuntimeError, "JavaScript execution error: out of memory");
    }

    auto pkey = partitions[i].partition_key.toString();
    JS::RootedValue pkey_str(ctx);
    auto pkey_str_ptr = JS_NewStringCopyN(ctx, pkey.data(), pkey.size());
    if (!pkey_str_ptr) {
      RAISE(kRuntimeError, "JavaScript execution error: out of memory");
    } else {
      pkey_str.setString(pkey_str_ptr);
    }
    JS_SetProperty(ctx, part_obj, "partition_key", pkey_str);

    auto time_begin = StringUtil::toString(
        partitions[i].time_begin.unixMicros());
    JS::RootedValue time_begin_str(ctx);
    auto time_begin_str_ptr = JS_NewStringCopyN(
        ctx,
        time_begin.data(),
        time_begin.size());
    if (!time_begin_str_ptr) {
      RAISE(kRuntimeError, "JavaScript execution error: out of memory");
    } else {
      time_begin_str.setString(time_begin_str_ptr);
    }
    JS_SetProperty(ctx, part_obj, "time_begin", time_begin_str);

    auto time_limit = StringUtil::toString(
        partitions[i].time_limit.unixMicros());
    JS::RootedValue time_limit_str(ctx);
    auto time_limit_str_ptr = JS_NewStringCopyN(
        ctx,
        time_limit.data(),
        time_limit.size());
    if (!time_limit_str_ptr) {
      RAISE(kRuntimeError, "JavaScript execution error: out of memory");
    } else {
      time_limit_str.setString(time_limit_str_ptr);
    }
    JS_SetProperty(ctx, part_obj, "time_limit", time_limit_str);
  }

  args.rval().setObject(*part_array);
  return true;
}

bool JavaScriptContext::executeMapReduce(
    JSContext* ctx,
    unsigned argc,
    JS::Value* vp) {
  auto args = JS::CallArgsFromVp(argc, vp);
  if (args.length() != 2 ||
      !args[0].isString() ||
      !args[1].isString()) {
    return false;
  }

  auto rt = JS_GetRuntime(ctx);
  auto self = (JavaScriptContext*) JS_GetRuntimePrivate(rt);
  if (!self) {
    return false;
  }

  if (self->task_builder_.get() == nullptr ||
      self->scheduler_.get() == nullptr) {
    return false;
  }

  auto jobs_json_cstr = JS_EncodeString(ctx, args[0].toString());
  String jobs_json(jobs_json_cstr);
  JS_free(ctx, jobs_json_cstr);

  auto job_id_cstr = JS_EncodeString(ctx, args[1].toString());
  String job_id(job_id_cstr);
  JS_free(ctx, job_id_cstr);

  try {
    auto jobs = json::parseJSON(jobs_json);
    auto task_shards = self->task_builder_->fromJSON(jobs.begin(), jobs.end());
    self->scheduler_->execute(task_shards);
  } catch (const StandardException& e) {
    self->storeError(e.what());
    return false;
  }

  args.rval().setBoolean(true);
  return true;
}

void JavaScriptContext::loadProgram(const String& program) {
  JSAutoRequest js_req(ctx_);
  JSAutoCompartment ac(ctx_, global_);

  JS::RootedValue rval(ctx_);

  JS::CompileOptions opts(ctx_);
  opts.setUTF8(true);
  opts.setFileAndLine("<mapreduce>", 1);

  std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf8to16;
  auto program_utf16 = utf8to16.from_bytes(
      program.data(),
      program.data() + program.length());

  if (!JS::Evaluate(
        ctx_,
        global_,
        opts,
        program_utf16.data(),
        program_utf16.length(),
        &rval)) {
    if (current_error_line_ > 0) {
      RAISEF(
          "JavaScriptError",
          "<$0:$1> $2",
          current_error_line_,
          current_error_column_,
          current_error_);
    } else {
      RAISE("JavaScriptError", current_error_);
    }
  }
}

void JavaScriptContext::loadClosure(
    const String& source,
    const String& globals,
    const String& params) {
  JSAutoRequest js_req(ctx_);
  JSAutoCompartment js_comp(ctx_, global_);

  std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf8to16;

  JS::AutoValueArray<3> argv(ctx_);
  auto source_utf16 = utf8to16.from_bytes(
      source.data(),
      source.data() + source.length());
  auto source_str_ptr = JS_NewUCStringCopyN(
      ctx_,
      source_utf16.data(),
      source_utf16.size());
  if (!source_str_ptr) {
    RAISE(kRuntimeError, "map function execution error: out of memory");
  } else {
    argv[0].setString(source_str_ptr);
  }

  auto globals_utf16 = utf8to16.from_bytes(
      globals.data(),
      globals.data() + globals.length());
  auto globals_str_ptr = JS_NewUCStringCopyN(
      ctx_,
      globals_utf16.data(),
      globals_utf16.size());
  if (!globals_str_ptr) {
    RAISE(kRuntimeError, "map function execution error: out of memory");
  } else {
    argv[1].setString(globals_str_ptr);
  }

  auto params_utf16 = utf8to16.from_bytes(
      params.data(),
      params.data() + params.length());
  auto params_str_ptr = JS_NewUCStringCopyN(
      ctx_,
      params_utf16.data(),
      params_utf16.size());
  if (!params_str_ptr) {
    RAISE(kRuntimeError, "map function execution error: out of memory");
  } else {
    argv[2].setString(params_str_ptr);
  }

  JS::RootedValue rval(ctx_);
  if (!JS_CallFunctionName(ctx_, global_, "__load_closure", argv, &rval)) {
    RAISE("JavaScriptError", current_error_);
  }
}

void JavaScriptContext::callMapFunction(
    const String& json_string,
    Vector<Pair<String, String>>* tuples) {
  JSAutoRequest js_req(ctx_);
  JSAutoCompartment js_comp(ctx_, global_);

  std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf8to16;
  auto json_string_utf16 = utf8to16.from_bytes(
      json_string.data(),
      json_string.data() + json_string.length());

  JS::RootedValue json(ctx_);
  if (JS_ParseJSON(ctx_, json_string_utf16.data(), json_string_utf16.length(), &json)) {
  } else {
    RAISE("JavaScriptError", current_error_);
  }

  JS::AutoValueArray<1> argv(ctx_);
  argv[0].set(json);

  JS::RootedValue rval(ctx_);
  if (!JS_CallFunctionName(ctx_, global_, "__fn", argv, &rval)) {
    RAISE("JavaScriptError", current_error_);
  }

  enumerateTuples(&rval, tuples);
}

void JavaScriptContext::callReduceFunction(
    const String& key,
    const Vector<String>& values,
    Vector<Pair<String, String>>* tuples) {
  JSAutoRequest js_req(ctx_);
  JSAutoCompartment js_comp(ctx_, global_);

  JS::AutoValueArray<2> argv(ctx_);

  std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf8to16;
  auto key_utf16 = utf8to16.from_bytes(key.data(), key.data() + key.length());
  auto key_str_ptr = JS_NewUCStringCopyN(ctx_, key_utf16.data(), key_utf16.size());
  if (!key_str_ptr) {
    RAISE(kRuntimeError, "reduce function execution error: out of memory");
  } else {
    argv[0].setString(key_str_ptr);
  }

  ReduceCollectionIter val_iter;
  val_iter.data = &values;
  val_iter.cur = 0;

  auto val_iter_obj_ptr = JS_NewObject(ctx_, &ReduceCollectionIter::kJSClass);
  if (!val_iter_obj_ptr) {
    RAISE(kRuntimeError, "reduce function execution error: out of memory");
  }

  JS::RootedObject val_iter_obj(ctx_, val_iter_obj_ptr);
  JS_SetPrivate(val_iter_obj, &val_iter);
  argv[1].setObject(*val_iter_obj);

  JS_DefineFunction(
      ctx_,
      val_iter_obj,
      "hasNext",
      &ReduceCollectionIter::hasNext,
      0,
      0);

  JS_DefineFunction(
      ctx_,
      val_iter_obj,
      "next",
      &ReduceCollectionIter::getNext,
      0,
      0);

  JS::RootedValue rval(ctx_);
  if (!JS_CallFunctionName(ctx_, global_, "__call_with_iter", argv, &rval)) {
    RAISEF(
        "JavaScriptError $0 for input $1/$2",
        current_error_,
        key,
        inspect(values));
  }

  enumerateTuples(&rval, tuples);
}

String JavaScriptContext::callSerializeFunction(
    const String& key,
    const String& value) {
  JSAutoRequest js_req(ctx_);
  JSAutoCompartment js_comp(ctx_, global_);

  JS::AutoValueArray<2> argv(ctx_);

  std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf8to16;
  auto key_utf16 = utf8to16.from_bytes(key.data(), key.data() + key.length());
  auto key_str_ptr = JS_NewUCStringCopyN(
      ctx_,
      key_utf16.data(),
      key_utf16.size());

  if (!key_str_ptr) {
    RAISE(kRuntimeError, "serialize function execution error: out of memory");
  } else {
    argv[0].setString(key_str_ptr);
  }

  auto value_utf16 = utf8to16.from_bytes(
      value.data(),
      value.data() + value.length());
  auto value_str_ptr = JS_NewUCStringCopyN(
      ctx_,
      value_utf16.data(),
      value_utf16.size());

  if (!value_str_ptr) {
    RAISE(kRuntimeError, "serialize function execution error: out of memory");
  } else {
    argv[1].setString(value_str_ptr);
  }

  JS::RootedValue rval(ctx_);
  if (!JS_CallFunctionName(ctx_, global_, "__fn", argv, &rval)) {
    RAISE("JavaScriptError", current_error_);
  }

  JS::RootedString res_rstr(ctx_, rval.toString());
  auto res_cstr = JS_EncodeStringToUTF8(ctx_, res_rstr);
  String res(res_cstr);
  JS_free(ctx_, res_cstr);

  return res;
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

    auto tkey_cstr = JS_EncodeStringToUTF8(ctx_, JS::RootedString(ctx_, tkey_jstr));
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

/*
Option<String> JavaScriptContext::getMapReduceJobJSON() {
  JSAutoRequest js_req(ctx_);
  JSAutoCompartment js_comp(ctx_, global_);

  JS::RootedValue job_def(ctx_);
  if (!JS_GetProperty(ctx_, global_, "__z1_mr_jobs", &job_def)) {
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
*/

JSClass JavaScriptContext::ReduceCollectionIter::kJSClass = {
  "global",
  JSCLASS_HAS_PRIVATE
};

bool JavaScriptContext::ReduceCollectionIter::hasNext(
    JSContext* ctx,
    unsigned argc,
    JS::Value* vp) {
  auto args = JS::CallArgsFromVp(argc, vp);
  if (!args.thisv().isObject()) {
     return false;
  }

  auto thisv = args.thisv();
  if (JS_GetClass(&thisv.toObject()) != &kJSClass) {
    return false;
  }

  auto iter = static_cast<ReduceCollectionIter*>(
      JS_GetPrivate(&thisv.toObject()));
  if (!iter) {
    return false;
  }

  if (iter->cur < iter->data->size()) {
    args.rval().set(JSVAL_TRUE);
  } else {
    args.rval().set(JSVAL_FALSE);
  }

  return true;
}

bool JavaScriptContext::ReduceCollectionIter::getNext(
    JSContext* ctx,
    unsigned argc,
    JS::Value* vp) {
  auto args = JS::CallArgsFromVp(argc, vp);
  if (!args.thisv().isObject()) {
     return false;
  }

  auto thisv = args.thisv();
  if (JS_GetClass(&thisv.toObject()) != &kJSClass) {
    return false;
  }

  auto iter = static_cast<ReduceCollectionIter*>(
      JS_GetPrivate(&thisv.toObject()));
  if (!iter) {
    return false;
  }

  if (iter->cur >= iter->data->size()) {
    return false;
  }

  const auto& value = (*iter->data)[iter->cur];
  ++iter->cur;

  std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf8to16;
  auto value_utf16 = utf8to16.from_bytes(
      value.data(),
      value.data() + value.length());
  auto value_str_ptr = JS_NewUCStringCopyN(
      ctx,
      value_utf16.data(),
      value_utf16.size());

  if (value_str_ptr) {
    args.rval().setString(value_str_ptr);
    return true;
  } else {
    return false;
  }
}

} // namespace eventql

