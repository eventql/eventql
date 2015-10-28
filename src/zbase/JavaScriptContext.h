/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include "stx/stdtypes.h"
#include "stx/autoref.h"
#include "stx/option.h"
#include <jsapi.h>

using namespace stx;

namespace zbase {

class JavaScriptContext : public RefCounted {
public:
  static const size_t kDefaultMemLimit = 1024 * 1024 * 128;
  static JSClass kGlobalJSClass;

  JavaScriptContext(size_t memlimit = kDefaultMemLimit);
  ~JavaScriptContext();

  void loadProgram(const String& program);

  void callMapFunction(
      const String& method_name,
      const String& json_string,
      Vector<Pair<String, String>>* tuples);

  void callReduceFunction(
      const String& method_name,
      const String& key,
      const Vector<String>& values,
      Vector<Pair<String, String>>* tuples);

  Option<String> getMapReduceJobJSON();

protected:

  struct ReduceCollectionIter {
    static JSClass kJSClass;
    static bool hasNext(JSContext* ctx, unsigned argc, JS::Value* vp);
    static bool getNext(JSContext* ctx, unsigned argc, JS::Value* vp);
    size_t cur;
    const Vector<String>* data;
  };

  void storeError(const String& error);
  void raiseError(const String& input);

  static void dispatchError(
      JSContext* ctx,
      const char* message,
      JSErrorReport* report);

  void storeLogline(const String& logline);

  static bool dispatchLog(
      JSContext* ctx,
      unsigned argc,
      JS::Value* vp);

  static bool listPartitions(
      JSContext* ctx,
      unsigned argc,
      JS::Value* vp);

  void enumerateTuples(
      JS::RootedValue* src,
      Vector<Pair<String, String>>* dst) const;

  JSRuntime* runtime_;
  JSContext* ctx_;
  JS::PersistentRooted<JSObject*> global_;
  String current_error_;
};

} // namespace zbase

