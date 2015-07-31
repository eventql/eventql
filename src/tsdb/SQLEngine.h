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
#include <chartsql/runtime/ResultFormat.h>
#include <tsdb/TSDBTableProvider.h>

namespace tsdb {
class TSDBService;

class SQLEngine {
public:

  static RefPtr<csql::TableProvider> tableProviderForNamespace(
      TSDBService* tsdb_node,
      const String& tsdb_namespace);

  static RefPtr<csql::QueryTreeNode> rewriteQuery(
      TSDBService* tsdb_node,
      const String& tsdb_namespace,
      RefPtr<csql::QueryTreeNode> query);

protected:

  static void replaceAllSequentialScansWithUnions(
      TSDBService* tsdb_node,
      const String& tsdb_namespace,
      RefPtr<csql::QueryTreeNode>* node);

  static void replaceSequentialScanWithUnion(
      TSDBService* tsdb_node,
      const String& tsdb_namespace,
      RefPtr<csql::QueryTreeNode>* node);

};

}
