# This file is part of the "libfnord" project
#   Copyright (c) 2015 Paul Asmuth
#
# FnordMetric is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License v3.0. You should have received a
# copy of the GNU General Public License along with this program. If not, see
# <http://www.gnu.org/licenses/>.
add_library(chartsql-lib OBJECT
    svalue.cc
    SFunction.cc
    format.cc
    #queryendpoint.cc
    #query.cc
    #queryservice.cc
    CSTableScan.cc
    CSTableScanProvider.cc
    backends/csv/csvbackend.cc
    backends/csv/csvinputstream.cc
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
    qtree/UnionNode.cc
    qtree/LimitNode.cc
    qtree/OrderByNode.cc
    qtree/DrawStatementNode.cc
    qtree/QueryTreeNode.cc
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
    runtime/Union.cc
    runtime/SelectExpression.cc
    runtime/EmptyTable.cc
    runtime/groupovertimewindow.cc
    runtime/execute.cc
    runtime/ExecutionContext.cc
    runtime/TableExpression.cc
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
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-chart>
    $<TARGET_OBJECTS:fnord-protobuf>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-metricdb>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:sstable>
    $<TARGET_OBJECTS:cstable>
    $<TARGET_OBJECTS:dproc>
    $<TARGET_OBJECTS:chartsql-lib>
    $<TARGET_OBJECTS:tsdb>
    runtime/Runtime_test.cc)

target_link_libraries(test-chartsql-runtime ${CMAKE_THREAD_LIBS_INIT} ${FNORD_LIBS})

add_executable(test-chartsql-parser
    $<TARGET_OBJECTS:fnord-base>
    $<TARGET_OBJECTS:fnord-chart>
    $<TARGET_OBJECTS:fnord-protobuf>
    $<TARGET_OBJECTS:fnord-http>
    $<TARGET_OBJECTS:fnord-json>
    $<TARGET_OBJECTS:fnord-metricdb>
    $<TARGET_OBJECTS:fnord-mdb>
    $<TARGET_OBJECTS:sstable>
    $<TARGET_OBJECTS:cstable>
    $<TARGET_OBJECTS:dproc>
    $<TARGET_OBJECTS:chartsql-lib>
    $<TARGET_OBJECTS:tsdb>
    parser/Parser_test.cc)

target_link_libraries(test-chartsql-parser ${CMAKE_THREAD_LIBS_INIT} ${FNORD_LIBS})
