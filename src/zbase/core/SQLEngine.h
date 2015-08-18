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
#include <csql/runtime/runtime.h>
#include <csql/runtime/ResultFormat.h>
#include <zbase/core/TSDBTableProvider.h>

namespace zbase {
class TSDBService;

class SQLEngine {
public:

  static RefPtr<csql::TableProvider> tableProviderForNamespace(
      PartitionMap* partition_map,
      CSTableIndex* cstable_index,
      const String& tsdb_namespace);

  static RefPtr<csql::QueryTreeNode> rewriteQuery(
      PartitionMap* partition_map,
      CSTableIndex* cstable_index,
      const String& tsdb_namespace,
      RefPtr<csql::QueryTreeNode> query);

protected:

  static void insertPartitionSubqueries(
      PartitionMap* partition_map,
      CSTableIndex* cstable_index,
      const String& tsdb_namespace,
      RefPtr<csql::QueryTreeNode>* node);

  static void replaceSequentialScanWithUnion(
      PartitionMap* partition_map,
      CSTableIndex* cstable_index,
      const String& tsdb_namespace,
      RefPtr<csql::QueryTreeNode>* node);

  static void shardGroupBy(
      PartitionMap* partition_map,
      CSTableIndex* cstable_index,
      const String& tsdb_namespace,
      RefPtr<csql::QueryTreeNode>* node);

};

}
