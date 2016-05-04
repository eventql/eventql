# This file is part of the "libfnord" project
#   Copyright (c) 2015 Paul Asmuth
#
# FnordMetric is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License v3.0. You should have received a
# copy of the GNU General Public License along with this program. If not, see
# <http://www.gnu.org/licenses/>.
file(GLOB PROTO_FILES **/*.proto)
STX_PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

add_library(chartsql STATIC
    svalue.cc
    SFunction.cc
    format.cc
    Transaction.cc
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
    parser/astnode.cc
    parser/astutil.cc
    parser/parser.cc
    parser/token.cc
    parser/tokenize.cc
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
    qtree/GroupByMergeNode.cc
    qtree/UnionNode.cc
    qtree/LimitNode.cc
    qtree/OrderByNode.cc
    qtree/QueryTreeNode.cc
    qtree/QueryTreeUtil.cc
    qtree/ShowTablesNode.cc
    qtree/DescribeTableNode.cc
    qtree/DrawStatementNode.cc
    qtree/ChartStatementNode.cc
    qtree/RemoteAggregateNode.cc
    runtime/ResultFormat.cc
    runtime/ValueExpression.cc
    runtime/ScratchMemory.cc
    runtime/runtime.cc
    runtime/symboltable.cc
    runtime/queryplannode.cc
    runtime/ValueExpressionBuilder.cc
    runtime/TableExpressionBuilder.cc
    runtime/defaultruntime.cc
    runtime/importstatement.cc
    runtime/tablerepository.cc
    runtime/queryplanbuilder.cc
    runtime/queryplan.cc
    runtime/tablescan.cc
    runtime/vm.cc
    runtime/compiler.cc
    runtime/ExecutionContext.cc
    runtime/ExecutionStrategy.cc
    runtime/QueryBuilder.cc
    runtime/LikePattern.cc
    runtime/charts/areachartbuilder.cc
    runtime/charts/barchartbuilder.cc
    runtime/charts/domainconfig.cc
    runtime/charts/drawstatement.cc
    runtime/charts/ChartStatement.cc
    runtime/charts/linechartbuilder.cc
    runtime/charts/pointchartbuilder.cc
    runtime/charts/seriesadapter.cc
    runtime/schedulers/LocalScheduler.cc
    tasks/Task.cc
    tasks/TaskFactory.cc
    tasks/TaskDAG.cc
    tasks/orderby.cc
    tasks/groupby.cc
    tasks/subquery.cc
    ${PROTO_SRCS}
    defaults.cc)

add_dependencies(chartsql stx-protobuf)
set(CSQL_LIBS chartsql cplot cstable stx-http stx-protobuf stx-json stx-base pcre)

add_executable(test-chartsql-runtime runtime/Runtime_test.cc)
target_link_libraries(test-chartsql-runtime ${CSQL_LIBS})

add_executable(test-chartsql-qtree qtree/QTree_test.cc)
target_link_libraries(test-chartsql-qtree ${CSQL_LIBS})

add_executable(test-chartsql-parser parser/Parser_test.cc)
target_link_libraries(test-chartsql-parser ${CSQL_LIBS})

