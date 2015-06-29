/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <tsdb/SQLEngine.h>
#include <chartsql/defaults.h>

namespace tsdb {

SQLEngine::SQLEngine() {
  csql::installDefaultSymbols(&symbol_table_);
}

RefPtr<csql::QueryTreeNode> SQLEngine::rewriteQuery(
    RefPtr<csql::QueryTreeNode> query) {
  return query;
}

RefPtr<csql::TableProvider> SQLEngine::defaultTableProvider() {
  return new csql::TableRepository();
}

}
