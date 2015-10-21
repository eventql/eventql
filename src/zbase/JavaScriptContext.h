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

  JavaScriptContext();
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

  void enumerateTuples(
      JS::RootedValue* src,
      Vector<Pair<String, String>>* dst) const;

  JSRuntime* runtime_;
  JSContext* ctx_;
  JS::PersistentRooted<JSObject*> global_;
};

} // namespace zbase

