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
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/sql/SFunction.h>

namespace csql {
class SymbolTableEntry;
class SValue;

class SymbolTableEntry {
public:

  SymbolTableEntry(
      const std::string& symbol,
      void (*method)(sql_txn*, void*, int, SValue*, SValue*));

  SymbolTableEntry(
      const std::string& symbol,
      void (*method)(sql_txn*, void*, int, SValue*, SValue*),
      size_t scratchpad_size,
      void (*free_method)(sql_txn*, void*));

  inline void call(
      sql_txn* ctx,
      void* scratchpad,
      int argc,
      SValue* argv,
      SValue* out) const {
    call_(ctx, scratchpad, argc, argv, out);
  }

  bool isAggregate() const;
  void (*getFnPtr() const)(sql_txn* ctx, void*, int, SValue*, SValue*);
  size_t getScratchpadSize() const;

protected:
  void (*call_)(sql_txn*, void*, int, SValue*, SValue*);
  const size_t scratchpad_size_;
};

class SymbolTable : public RefCounted {
public:

  SymbolTableEntry const* lookupSymbol(const std::string& symbol) const;

  SFunction lookup(const String& symbol) const;

  bool isAggregateFunction(const String& symbol) const;

  void registerSymbol(
      const std::string& symbol,
      void (*method)(sql_txn*, void*, int, SValue*, SValue*));

  void registerSymbol(
      const std::string& symbol,
      void (*method)(sql_txn* ctx, void*, int, SValue*, SValue*),
      size_t scratchpad_size,
      void (*free_method)(sql_txn* ctx, void*));

  void registerFunction(
      const String& symbol,
      void (*fn)(sql_txn*, int, SValue*, SValue*));

  void registerFunction(
      const String& symbol,
      AggregateFunction fn);

  void registerFunction(
      const String& symbol,
      SFunction fn);

protected:
  std::unordered_map<std::string, SymbolTableEntry> symbols_;
  HashMap<String, SFunction> syms_;
};

}
