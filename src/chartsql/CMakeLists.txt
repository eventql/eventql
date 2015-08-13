# This file is part of the "libfnord" project
#   Copyright (c) 2015 Paul Asmuth
#
# FnordMetric is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License v3.0. You should have received a
# copy of the GNU General Public License along with this program. If not, see
# <http://www.gnu.org/licenses/>.
add_library(chartsql STATIC
    svalue.cc
    SFunction.cc
    format.cc
    #queryendpoint.cc
    #query.cc
    #queryservice.cc
    CSTableScan.cc
    CSTableScanProvider.cc
    backends/csv/csvbackend.cc
    backends/csv/csvtableref.cc
    backends/mysql/mysqlbackend.cc
    backends/mysql/mysqlconnection.cc
    backends/mysql/mysqltableref.cc
    #backends/metricservice/metrictableref.cc
    #backends/metricservice/metrictablerepository.cc
    expressions/aggregate.cc
    expressions/boolean.cc
    expressions/datetime.cc
    expressions/math.cc
    parser/astnode.cc
    parser/astutil.cc
    parser/parser.cc
    parser/token.cc
    parser/tokenize.cc
    qtree/CallExpressionNode.cc
    qtree/ColumnReferenceNode.cc
    qtree/SequentialScanNode.cc
    qtree/SelectExpressionNode.cc
    qtree/LiteralExpressionNode.cc
    qtree/IfExpressionNode.cc
    qtree/TableExpressionNode.cc
    qtree/SelectListNode.cc
    qtree/GroupByNode.cc
    qtree/GroupByMergeNode.cc
    qtree/UnionNode.cc
    qtree/LimitNode.cc
    qtree/OrderByNode.cc
    qtree/QueryTreeNode.cc
    qtree/ShowTablesNode.cc
    qtree/DescribeTableNode.cc
    qtree/DrawStatementNode.cc
    qtree/ChartStatementNode.cc
    runtime/ASCIITableFormat.cc
    runtime/JSONSSEStreamFormat.cc
    runtime/JSONResultFormat.cc
    runtime/ValueExpression.cc
    runtime/ScratchMemory.cc
    runtime/runtime.cc
    runtime/symboltable.cc
    runtime/queryplannode.cc
    runtime/ValueExpressionBuilder.cc
    runtime/TableExpressionBuilder.cc
    runtime/defaultruntime.cc
    runtime/DefaultExecutionPlan.cc
    runtime/importstatement.cc
    runtime/tablerepository.cc
    runtime/queryplanbuilder.cc
    runtime/queryplan.cc
    runtime/tablescan.cc
    runtime/orderby.cc
    runtime/limitclause.cc
    runtime/GroupBy.cc
    runtime/GroupByMerge.cc
    runtime/Union.cc
    runtime/SelectExpression.cc
    runtime/EmptyTable.cc
    runtime/groupovertimewindow.cc
    runtime/ShowTablesStatement.cc
    runtime/DescribeTableStatement.cc
    runtime/execute.cc
    runtime/ExecutionContext.cc
    runtime/ExecutionStrategy.cc
    runtime/TableExpression.cc
    runtime/QueryBuilder.cc
    runtime/charts/areachartbuilder.cc
    runtime/charts/barchartbuilder.cc
    runtime/charts/domainconfig.cc
    runtime/charts/drawstatement.cc
    runtime/charts/ChartStatement.cc
    runtime/charts/linechartbuilder.cc
    runtime/charts/pointchartbuilder.cc
    runtime/charts/seriesadapter.cc
    defaults.cc)

add_executable(test-chartsql-runtime
    $<TARGET_OBJECTS:dproc>
    $<TARGET_OBJECTS:tsdb-client>
    runtime/Runtime_test.cc)

target_link_libraries(test-chartsql-runtime chartsql cplot stx-http stx-protobuf stx-json stx-base)

add_executable(test-chartsql-parser
    $<TARGET_OBJECTS:dproc>
    $<TARGET_OBJECTS:tsdb-client>
    parser/Parser_test.cc)

target_link_libraries(test-chartsql-parser chartsql cplot stx-http stx-protobuf stx-json stx-base)
