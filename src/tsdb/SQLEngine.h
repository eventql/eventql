/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <chartsql/runtime/runtime.h>
#include <tsdb/TSDBTableProvider.h>

namespace tsdb {

class SQLEngine : public csql::Runtime {
public:

  SQLEngine();

  RefPtr<csql::TableProvider> tableProviderForNamespace(
      const String& tsdb_namespace);

protected:

  RefPtr<csql::QueryTreeNode> rewriteQuery(
      RefPtr<csql::QueryTreeNode> query) override;

  RefPtr<csql::TableProvider> defaultTableProvider() override;

};

}
