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
#include <eventql/sql/runtime/tablerepository.h>
#include <eventql/util/exception.h>
#include <eventql/util/uri.h>

namespace csql {

TableRef* TableRepository::getTableRef(const std::string& table_name) const {
  auto iter = table_refs_.find(table_name);

  if (iter == table_refs_.end()) {
    return nullptr;
  } else {
    return iter->second.get();
  }
}

void TableRepository::addTableRef(
    const std::string& table_name,
    std::unique_ptr<TableRef>&& table_ref) {
  table_refs_[table_name] = std::move(table_ref);
}

void TableRepository::import(
    const std::vector<std::string>& tables,
    const std::string& source_uri_raw,
    const std::vector<std::unique_ptr<Backend>>& backends) {
  util::URI source_uri(source_uri_raw);

  for (const auto& backend : backends) {
    std::vector<std::unique_ptr<TableRef>> tbl_refs;

    if (backend->openTables(tables, source_uri, &tbl_refs)) {
      if (tbl_refs.size() != tables.size()) {
        RAISE(
            kRuntimeError,
            "openTables failed for '%s'\n",
            source_uri.toString().c_str());
      }

      for (int i = 0; i < tables.size(); ++i) {
        addTableRef(tables[i], std::move(tbl_refs[i]));
      }

      return;
    }
  }

  RAISE(
      kRuntimeError,
      "no backend found for '%s'\n",
      source_uri.toString().c_str());
}

void TableRepository::addProvider(RefPtr<TableProvider> provider) {
  providers_.emplace_back(provider);
}

void TableRepository::listTables(
    Function<void (const TableInfo& table)> fn) const {
  for (const auto& p : providers_) {
    p->listTables(fn);
  }
}

Option<TableInfo> TableRepository::describe(const String& table_name) const {
  for (const auto& p : providers_) {
    auto tbl = p->describe(table_name);
    if (!tbl.isEmpty()) {
      return tbl;
    }
  }

  return None<TableInfo>();
}

Option<ScopedPtr<TableExpression>> TableRepository::buildSequentialScan(
      Transaction* ctx,
      RefPtr<SequentialScanNode> seqscan) const {
  for (const auto& provider : providers_) {
    auto expr = provider->buildSequentialScan(ctx, seqscan);
    if (!expr.isEmpty()) {
      return expr;
    }
  }

  RAISEF(kNotFoundError, "table not found: '$0'", seqscan->tableName());
}

}
