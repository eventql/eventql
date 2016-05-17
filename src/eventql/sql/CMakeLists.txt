# Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
# Authors:
#   - Paul Asmuth <paul@zscale.io>
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU Affero General Public License ("the license") as
# published by the Free Software Foundation, either version 3 of the License,
# or any later version.
#
# In accordance with Section 7(e) of the license, the licensing of the Program
# under the license does not imply a trademark license. Therefore any rights,
# title and interest in our trademarks remain entirely with us.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the license for more details.
#
# You can be released from the requirements of the license by purchasing a
# commercial license. Buying such a license is mandatory as soon as you develop
# commercial activities involving this program without disclosing the source
# code of your own applications

add_library(chartsql STATIC
    svalue.cc
    SFunction.cc
    format.cc
    transaction.cc
    #queryendpoint.cc
    #query.cc
    #queryservice.cc
    CSTableScan.cc
    CSTableScanProvider.cc
    backends/csv/CSVInputStream.cc
    backends/csv/CSVTableProvider.cc
    backends/csv/CSVTableScan.cc
    expressions/aggregate.cc
    expressions/boolean.cc
    expressions/conversion.cc
    expressions/datetime.cc
    expressions/internal.cc
    expressions/math.cc
    expressions/string.cc
    expressions/table/select.cc
    expressions/table/subquery.cc
    expressions/table/orderby.cc
    expressions/table/tablescan.cc
    expressions/table/show_tables.cc
    expressions/table/describe_table.cc
    expressions/table/limit.cc
    expressions/table/groupby.cc
    expressions/table/nested_loop_join.cc
    extensions/chartsql/areachartbuilder.cc
    extensions/chartsql/barchartbuilder.cc
    extensions/chartsql/chart_expression.cc
    extensions/chartsql/domainconfig.cc
    extensions/chartsql/linechartbuilder.cc
    extensions/chartsql/pointchartbuilder.cc
    extensions/chartsql/seriesadapter.cc
    parser/astnode.cc
    parser/astutil.cc
    parser/parser.cc
    parser/token.cc
    parser/tokenize.cc
    qtree/qtree_coder.cc
    qtree/CallExpressionNode.cc
    qtree/ColumnReferenceNode.cc
    qtree/JoinCondition.cc
    qtree/JoinNode.cc
    qtree/SequentialScanNode.cc
    qtree/SubqueryNode.cc
    qtree/SelectExpressionNode.cc
    qtree/LiteralExpressionNode.cc
    qtree/IfExpressionNode.cc
    qtree/RegexExpressionNode.cc
    qtree/LikeExpressionNode.cc
    qtree/TableExpressionNode.cc
    qtree/SelectListNode.cc
    qtree/GroupByNode.cc
    qtree/UnionNode.cc
    qtree/LimitNode.cc
    qtree/OrderByNode.cc
    qtree/JoinNode.cc
    qtree/QueryTreeNode.cc
    qtree/QueryTreeUtil.cc
    qtree/ShowTablesNode.cc
    qtree/DescribeTableNode.cc
    qtree/DrawStatementNode.cc
    qtree/ChartStatementNode.cc
    runtime/ResultFormat.cc
    runtime/ValueExpression.cc
    runtime/ScratchMemory.cc
    runtime/runtime.cc
    runtime/symboltable.cc
    runtime/ValueExpressionBuilder.cc
    runtime/defaultruntime.cc
    runtime/tablerepository.cc
    runtime/queryplanbuilder.cc
    query_plan.cc
    runtime/vm.cc
    runtime/compiler.cc
    runtime/ExecutionContext.cc
    runtime/QueryBuilder.cc
    runtime/LikePattern.cc
    #runtime/charts/areachartbuilder.cc
    #runtime/charts/barchartbuilder.cc
    #runtime/charts/domainconfig.cc
    #runtime/charts/drawstatement.cc
    #runtime/charts/ChartStatement.cc
    #runtime/charts/linechartbuilder.cc
    #runtime/charts/pointchartbuilder.cc
    #runtime/charts/seriesadapter.cc
    result_cursor.cc
    scheduler.cc
    defaults.cc)

add_dependencies(chartsql stx-protobuf)
set(CSQL_LIBS chartsql cplot cstable stx-http stx-protobuf stx-json stx-base pcre)

add_executable(test-chartsql-runtime runtime/Runtime_test.cc)
target_link_libraries(test-chartsql-runtime ${CSQL_LIBS})

add_executable(test-chartsql-qtree qtree/QTree_test.cc)
target_link_libraries(test-chartsql-qtree ${CSQL_LIBS})

add_executable(test-chartsql-parser parser/Parser_test.cc)
target_link_libraries(test-chartsql-parser ${CSQL_LIBS})

