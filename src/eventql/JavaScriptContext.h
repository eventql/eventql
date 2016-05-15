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
#pragma once
#include "eventql/util/stdtypes.h"
#include "eventql/util/autoref.h"
#include "eventql/util/option.h"
#include "eventql/core/TSDBService.h"
#include "eventql/mapreduce/MapReduceTaskBuilder.h"
#include "eventql/mapreduce/MapReduceScheduler.h"
#include <jsapi.h>

using namespace stx;

namespace eventql {

class JavaScriptContext : public RefCounted {
public:
  static const size_t kDefaultMemLimit = 1024 * 1024 * 128;
  static JSClass kGlobalJSClass;

  JavaScriptContext(
      const String& customer,
      RefPtr<MapReduceJobSpec> job,
      TSDBService* tsdb,
      RefPtr<MapReduceTaskBuilder> task_builder,
      RefPtr<MapReduceScheduler> scheduler,
      size_t memlimit = kDefaultMemLimit);

  ~JavaScriptContext();

  void loadProgram(const String& program);

  void loadClosure(
      const String& source,
      const String& globals,
      const String& params);

  void callMapFunction(
      const String& json_string,
      Vector<Pair<String, String>>* tuples);

  void callReduceFunction(
      const String& key,
      const Vector<String>& values,
      Vector<Pair<String, String>>* tuples);

  String callSerializeFunction(
      const String& key,
      const String& value);

protected:

  struct ReduceCollectionIter {
    static JSClass kJSClass;
    static bool hasNext(JSContext* ctx, unsigned argc, JS::Value* vp);
    static bool getNext(JSContext* ctx, unsigned argc, JS::Value* vp);
    size_t cur;
    const Vector<String>* data;
  };

  void storeError(
      const String& error,
      size_t line = 0,
      size_t column = 0);

  static void dispatchError(
      JSContext* ctx,
      const char* message,
      JSErrorReport* report);

  static bool dispatchLog(
      JSContext* ctx,
      unsigned argc,
      JS::Value* vp);

  static bool returnResult(
      JSContext* ctx,
      unsigned argc,
      JS::Value* vp);

  static bool listPartitions(
      JSContext* ctx,
      unsigned argc,
      JS::Value* vp);

  static bool executeMapReduce(
      JSContext* ctx,
      unsigned argc,
      JS::Value* vp);

  void enumerateTuples(
      JS::RootedValue* src,
      Vector<Pair<String, String>>* dst) const;

  String customer_;
  RefPtr<MapReduceJobSpec> job_;
  TSDBService* tsdb_;
  RefPtr<MapReduceTaskBuilder> task_builder_;
  RefPtr<MapReduceScheduler> scheduler_;
  JSRuntime* runtime_;
  JSContext* ctx_;
  JS::PersistentRooted<JSObject*> global_;
  String current_error_;
  size_t current_error_line_;
  size_t current_error_column_;
};

} // namespace eventql

