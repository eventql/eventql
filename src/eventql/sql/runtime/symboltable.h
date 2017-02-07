/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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

class SymbolTableEntry {
public:

  SymbolTableEntry(const std::string& function_name, SFunction fun);

  const SFunction* getFunction() const;
  const std::string& getSymbol() const;

protected:
  SFunction fun_;
  std::string symbol_;
};

class SymbolTable : public RefCounted {
public:

  void registerFunction(const std::string& function_name, SFunction fn);
  void registerImplicitConversion(SType source_type, SType target_type);

  ReturnCode resolve(
      const std::string& function_name,
      const std::vector<SType>& arguments,
      const SymbolTableEntry** entry,
      bool allow_conversion = true) const;

  const SymbolTableEntry* lookup(const std::string& symbol) const;

  bool isAggregateFunction(const std::string& function_name) const;
  bool hasImplicitConversion(SType source, SType target) const;

protected:
  mutable std::mutex mutex_;
  std::map<std::string, std::vector<const SymbolTableEntry*>> functions_;
  std::map<std::string, std::unique_ptr<SymbolTableEntry>> symbols_;
  std::map<SType, std::set<SType>> implicit_conversions_;
};

}
