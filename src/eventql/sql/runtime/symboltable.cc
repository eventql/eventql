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

void SymbolTable::registerFunction(
    const std::string& function_name,
    SFunction fn) {
  std::unique_lock<std::mutex> lk(mutex_);
  symbols_[function_name].emplace_back(fn);
}

const std::vector<SFunction>* SymbolTable::lookup(
    const std::string& symbol) const {
  std::unique_lock<std::mutex> lk(mutex_);

  std::string symbol_downcase = symbol;
  StringUtil::toLower(&symbol_downcase);

  auto iter = symbols_.find(symbol_downcase);
  if (iter == symbols_.end()) {
    return nullptr;
  } else {
    return &iter->second;
  }
}

bool SymbolTable::isAggregateFunction(const std::string& symbol) const {
  std::unique_lock<std::mutex> lk(mutex_);

  std::string symbol_downcase = symbol;
  StringUtil::toLower(&symbol_downcase);

  auto iter = symbols_.find(symbol_downcase);
  if (iter != symbols_.end()) {
    for (const auto& f : iter->second) {
      if (f.type == FN_AGGREGATE) {
        return true;
      }
    }
  }

  return false;
}

}
