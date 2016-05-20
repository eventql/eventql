## Process this file with automake to produce Makefile.in

if HAVE_ZLIB
ZLIB_DEF = -DHAVE_ZLIB=1
else
ZLIB_DEF =
endif

if HAVE_PTHREAD
PTHREAD_DEF = -DHAVE_PTHREAD=1
else
PTHREAD_DEF =
endif

AM_CXXFLAGS = $(PTHREAD_CFLAGS) $(PTHREAD_DEF) $(ZLIB_DEF) -std=c++0x -ftemplate-depth=500 -mno-omit-leaf-frame-pointer -fno-omit-frame-pointer -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -Wdelete-non-virtual-dtor -Wno-predefined-identifier-outside-function -O3 -g
AM_CFLAGS = $(PTHREAD_CFLAGS) $(PTHREAD_DEF) $(ZLIB_DEF) -std=c11 -mno-omit-leaf-frame-pointer -fno-omit-frame-pointer -Wall -pedantic -O3 -g
AM_LDFLAGS = $(PTHREAD_CFLAGS)

MAINTAINERCLEANFILES =   \
  Makefile.in

#bin_PROGRAMS = evql evqld
lib_LIBRARIES = libsql.a

libsql_a_SOURCES = \
    eventql/sql/svalue.cc \
    eventql/sql/SFunction.cc \
    eventql/sql/format.cc \
    eventql/sql/transaction.cc \
    eventql/sql/CSTableScan.cc \
    eventql/sql/CSTableScanProvider.cc \
    eventql/sql/backends/csv/CSVInputStream.cc \
    eventql/sql/backends/csv/CSVTableProvider.cc \
    eventql/sql/backends/csv/CSVTableScan.cc \
    eventql/sql/expressions/aggregate.cc \
    eventql/sql/expressions/boolean.cc \
    eventql/sql/expressions/conversion.cc \
    eventql/sql/expressions/datetime.cc \
    eventql/sql/expressions/internal.cc \
    eventql/sql/expressions/math.cc \
    eventql/sql/expressions/string.cc \
    eventql/sql/expressions/table/select.cc \
    eventql/sql/expressions/table/subquery.cc \
    eventql/sql/expressions/table/orderby.cc \
    eventql/sql/expressions/table/tablescan.cc \
    eventql/sql/expressions/table/show_tables.cc \
    eventql/sql/expressions/table/describe_table.cc \
    eventql/sql/expressions/table/limit.cc \
    eventql/sql/expressions/table/groupby.cc \
    eventql/sql/expressions/table/nested_loop_join.cc \
    eventql/sql/extensions/chartsql/areachartbuilder.cc \
    eventql/sql/extensions/chartsql/barchartbuilder.cc \
    eventql/sql/extensions/chartsql/chart_expression.cc \
    eventql/sql/extensions/chartsql/domainconfig.cc \
    eventql/sql/extensions/chartsql/linechartbuilder.cc \
    eventql/sql/extensions/chartsql/pointchartbuilder.cc \
    eventql/sql/extensions/chartsql/seriesadapter.cc \
    eventql/sql/parser/astnode.cc \
    eventql/sql/parser/astutil.cc \
    eventql/sql/parser/parser.cc \
    eventql/sql/parser/token.cc \
    eventql/sql/parser/tokenize.cc \
    eventql/sql/qtree/qtree_coder.cc \
    eventql/sql/qtree/CallExpressionNode.cc \
    eventql/sql/qtree/ColumnReferenceNode.cc \
    eventql/sql/qtree/JoinCondition.cc \
    eventql/sql/qtree/JoinNode.cc \
    eventql/sql/qtree/SequentialScanNode.cc \
    eventql/sql/qtree/SubqueryNode.cc \
    eventql/sql/qtree/SelectExpressionNode.cc \
    eventql/sql/qtree/LiteralExpressionNode.cc \
    eventql/sql/qtree/IfExpressionNode.cc \
    eventql/sql/qtree/RegexExpressionNode.cc \
    eventql/sql/qtree/LikeExpressionNode.cc \
    eventql/sql/qtree/TableExpressionNode.cc \
    eventql/sql/qtree/SelectListNode.cc \
    eventql/sql/qtree/GroupByNode.cc \
    eventql/sql/qtree/UnionNode.cc \
    eventql/sql/qtree/LimitNode.cc \
    eventql/sql/qtree/OrderByNode.cc \
    eventql/sql/qtree/JoinNode.cc \
    eventql/sql/qtree/QueryTreeNode.cc \
    eventql/sql/qtree/QueryTreeUtil.cc \
    eventql/sql/qtree/ShowTablesNode.cc \
    eventql/sql/qtree/DescribeTableNode.cc \
    eventql/sql/qtree/DrawStatementNode.cc \
    eventql/sql/qtree/ChartStatementNode.cc \
    eventql/sql/runtime/ValueExpression.cc \
    eventql/sql/runtime/ScratchMemory.cc \
    eventql/sql/runtime/runtime.cc \
    eventql/sql/runtime/symboltable.cc \
    eventql/sql/runtime/ValueExpressionBuilder.cc \
    eventql/sql/runtime/defaultruntime.cc \
    eventql/sql/runtime/tablerepository.cc \
    eventql/sql/runtime/queryplanbuilder.cc \
    eventql/sql/query_plan.cc \
    eventql/sql/runtime/vm.cc \
    eventql/sql/runtime/compiler.cc \
    eventql/sql/runtime/QueryBuilder.cc \
    eventql/sql/runtime/LikePattern.cc \
    eventql/sql/scheduler/execution_context.cc \
    eventql/sql/result_cursor.cc \
    eventql/sql/scheduler.cc \
    eventql/sql/defaults.cc

#protoc_LDADD = $(PTHREAD_LIBS) libprotobuf.la libprotoc.la
#protoc_SOURCES = google/protobuf/compiler/main.cc

