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
#include <algorithm>
#include <stdlib.h>
#include <assert.h>
#include <string>
#include "symboltable.h"
#include <eventql/util/exception.h>

namespace csql {

SymbolTableEntry::SymbolTableEntry(
    const std::string& function_name,
    SFunction fun) :
    fun_(fun) {
  symbol_ = function_name + "#" + sql_typename(fun_.return_type) + "/";

  for (const auto& t : fun_.arg_types) {
    symbol_ += sql_typename(t) + ";";
  }
}

const SFunction* SymbolTableEntry::getFunction() const {
  return &fun_;
}

const std::string& SymbolTableEntry::getSymbol() const {
  return symbol_;
}

void SymbolTable::registerFunction(
    const std::string& function_name,
    SFunction fn) {
  std::unique_lock<std::mutex> lk(mutex_);
  std::unique_ptr<SymbolTableEntry> entry(
      new SymbolTableEntry(function_name, fn));

  auto symbol = entry->getSymbol();
  functions_[function_name].emplace_back(entry.get());
  symbols_.emplace(symbol, std::move(entry));
}

void SymbolTable::registerImplicitConversion(
    SType source_type,
    SType target_type) {
  std::unique_lock<std::mutex> lk(mutex_);
  implicit_conversions_[source_type].insert(target_type);
}

ReturnCode SymbolTable::resolve(
    const std::string& function_name,
    const std::vector<SType>& arguments,
    const SymbolTableEntry** entry,
    bool allow_conversion /* = true */) const {
  std::string function_name_downcase = function_name;
  StringUtil::toLower(&function_name_downcase);

  std::vector<const SymbolTableEntry*> candidates;
  {
    std::unique_lock<std::mutex> lk(mutex_);
    auto iter = functions_.find(function_name_downcase);
    if (iter == functions_.end()) {
      return ReturnCode::errorf("EARG", "method not found: $0", function_name);
    }

    candidates = iter->second;
  }

  const SymbolTableEntry* match = nullptr;

  /* scan the candidates looking for an exact match */
  for (const auto& candidate : candidates) {
    auto candidate_fn = candidate->getFunction();
    if (candidate_fn->arg_types.size() != arguments.size()) {
      continue;
    }

    match = candidate;
    for (size_t i = 0; i < arguments.size(); ++i) {
      if (candidate_fn->arg_types[i] != arguments[i]) {
        match = nullptr;
        break;
      }
    }

    if (match) {
      break;
    }
  }

  /* scan the candidates looking for a match using implicit conversions */
  if (!match && allow_conversion) {
    for (const auto& candidate : candidates) {
      auto candidate_fn = candidate->getFunction();
      if (candidate_fn->arg_types.size() != arguments.size()) {
        continue;
      }

      match = candidate;
      for (size_t i = 0; i < arguments.size(); ++i) {
        if (!hasImplicitConversion(arguments[i], candidate_fn->arg_types[i])) {
          match = nullptr;
          break;
        }
      }

      if (match) {
        break;
      }
    }
  }

  if (match) {
    *entry = match;
    return ReturnCode::success();
  } else {
    std::vector<std::string> expected_types;
    for (const auto& candidate : candidates) {
      std::vector<std::string> candidate_types;
      auto candidate_fn = candidate->getFunction();

      for (const auto& type : candidate_fn->arg_types) {
        candidate_types.emplace_back(sql_typename(type));
      }

      expected_types.emplace_back(
          function_name + "<" + StringUtil::join(candidate_types, ", ") + ">");
    }

    std::vector<std::string> actual_types;
    for (auto type : arguments) {
      actual_types.emplace_back(sql_typename(type));
    }

    return ReturnCode::errorf(
        "ERUNTIME",
        "type error for $0<$1>; expected: $2",
        function_name,
        StringUtil::join(actual_types, ", "),
        StringUtil::join(expected_types, " or "));
  }
}

const SymbolTableEntry* SymbolTable::lookup(const std::string& symbol) const {
  std::string symbol_downcase = symbol;
  StringUtil::toLower(&symbol_downcase);

  std::unique_lock<std::mutex> lk(mutex_);
  auto iter = symbols_.find(symbol_downcase);
  if (iter == symbols_.end()) {
    return nullptr;
  } else {
    return iter->second.get();
  }
}

bool SymbolTable::isAggregateFunction(const std::string& function_name) const {
  std::unique_lock<std::mutex> lk(mutex_);

  std::string function_name_downcase = function_name;
  StringUtil::toLower(&function_name_downcase);

  auto iter = functions_.find(function_name_downcase);
  if (iter != functions_.end()) {
    for (const auto& f : iter->second) {
      if (f->getFunction()->type == FN_AGGREGATE) {
        return true;
      }
    }
  }

  return false;
}

bool SymbolTable::hasImplicitConversion(SType source, SType target) const {
  std::unique_lock<std::mutex> lk(mutex_);

  auto iter = implicit_conversions_.find(source);
  if (iter == implicit_conversions_.end()) {
    return false;
  } else {
    return iter->second.count(target) > 0;
  }
}

} // namespace csql

