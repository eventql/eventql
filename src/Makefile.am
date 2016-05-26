## Process this file with automake to produce Makefile.in

MAINTAINERCLEANFILES =   \
  Makefile.in

EXTRA_DIST = \
  eventql/util/assets.sh \
  eventql/util/charts/testdata \
  eventql/sql/testdata \
  eventql/mapreduce/prelude.js

# we'll fill those later
bin_PROGRAMS =
noinst_LIBRARIES =
dist_noinst_DATA =
BUILT_SOURCES =
TESTS =
check_PROGRAMS =


################# DEPENDENCIES ####################
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

if HAVE_SYSLOG_H
SYSLOG_DEF = -DHAVE_SYSLOG_H=1
else
SYSLOG_DEF =
endif

if HAVE_GETHOSTBYNAME_R
GETHOSTBYNAME_R_DEF = -DHAVE_GETHOSTBYNAME_R=1
else
GETHOSTBYNAME_R_DEF =
endif

SPIDERMONKEY_CFLAGS = -I$(top_builddir)/deps/3rdparty/spidermonkey/out/dist/include
SPIDERMONKEY_LDADD = ${top_builddir}/deps/3rdparty/spidermonkey/out/dist/lib/libjs_static.a

ZOOKEEPER_CFLAGS = -I$(top_srcdir)/deps/3rdparty/zookeeper/source/include -I$(top_srcdir)/deps/3rdparty/zookeeper/source/generated
ZOOKEEPER_LDADD = ${top_builddir}/deps/3rdparty/zookeeper/libzkmt.a ${top_builddir}/deps/3rdparty/zookeeper/libhashtable.a

PCRE_CFLAGS = -I$(top_srcdir)/deps/3rdparty/pcre -DHAVE_PCRE=1
PCRE_LDADD = ${top_builddir}/deps/3rdparty/pcre/libpcre.a

LINENOISE_CFLAGS = -I$(top_srcdir)/deps/3rdparty
LINENOISE_LDADD = ${top_builddir}/deps/3rdparty/linenoise/liblinenoise.a

SIMDCOMP_CFLAGS = -I$(top_srcdir)/deps/3rdparty
SIMDCOMP_LDADD = ${top_builddir}/deps/3rdparty/libsimdcomp/libsimdcomp.a

PROTOBUF_CFLAGS = -I$(top_srcdir)/deps/3rdparty/protobuf
PROTOBUF_LDADD = ${top_builddir}/deps/3rdparty/protobuf/libprotobuf.a

LMDB_CFLAGS = -I$(top_srcdir)/deps/3rdparty
LMDB_LDADD = ${top_builddir}/deps/3rdparty/liblmdb/liblmdb.a

AM_CXXFLAGS = $(PCRE_CFLAGS) $(SPIDERMONKEY_CFLAGS) $(ZOOKEEPER_CFLAGS) $(SIMDCOMP_CFLAGS) $(LINENOISE_CLFAGS) $(PROTOBUF_CFLAGS) $(PTHREAD_CFLAGS) $(PTHREAD_DEF) $(SYSLOG_DEF) $(ZLIB_DEF) $(GETHOSTBYNAME_R_DEF) -std=c++0x -ftemplate-depth=500 -mno-omit-leaf-frame-pointer -fno-omit-frame-pointer -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -Wdelete-non-virtual-dtor -Wno-predefined-identifier-outside-function -g -I@builddir@
AM_CFLAGS = $(PCRE_CFLAGS) $(SPIDERMONKEY_CFLAGS) $(ZOOKEEPER_CFLAGS) $(SIMDCOMP_CFLAGS) $(LINENOISE_CLFAGS) $(PROTOBUF_CFLAGS) $(PTHREAD_CFLAGS) $(PTHREAD_DEF) $(SYSLOG_DEF) $(ZLIB_DEF) $(GETHOSTBYNAME_R_DEF) -std=c11 -mno-omit-leaf-frame-pointer -fno-omit-frame-pointer -Wall -pedantic -g
AM_LDFLAGS = $(PTHREAD_CFLAGS)


################# PROTOBUFs ####################
evql_protos = \
    eventql/AnalyticsSession.proto \
    eventql/config/namespace_config_protos.proto \
    eventql/db/ClusterConfig.proto \
    eventql/db/LogPartitionCompactionState.proto \
    eventql/db/PartitionInfo.proto \
    eventql/db/PartitionState.proto \
    eventql/db/RecordEnvelope.proto \
    eventql/db/ReplicationState.proto \
    eventql/db/TableConfig.proto \
    eventql/db/TSDBNodeConfig.proto \
    eventql/db/TSDBTableScanSpec.proto \
    eventql/EventScanParams.proto \
    eventql/LogfileScanParams.proto \
    eventql/RemoteTSDBScanParams.proto

BUILT_SOURCES += $(evql_protos:.proto=.pb.h)
dist_noinst_DATA += $(evql_protos)

%.pb.cc %.pb.h: %.proto
	$(top_builddir)/deps/3rdparty/protobuf/protoc --proto_path=$(srcdir) --cpp_out=$(builddir) $^


################# LIB: evql_sql.a ####################
evql_sql_a_SOURCES = \
    eventql/sql/csql.h \
    eventql/sql/CSTableScan.cc \
    eventql/sql/CSTableScan.h \
    eventql/sql/CSTableScanProvider.cc \
    eventql/sql/CSTableScanProvider.h \
    eventql/sql/SFunction.cc \
    eventql/sql/SFunction.h \
    eventql/sql/TableInfo.h \
    eventql/sql/abi.h \
    eventql/sql/backends/backend.h \
    eventql/sql/backends/csv/CSVInputStream.cc \
    eventql/sql/backends/csv/CSVInputStream.h \
    eventql/sql/backends/csv/CSVTableProvider.cc \
    eventql/sql/backends/csv/CSVTableProvider.h \
    eventql/sql/backends/csv/CSVTableScan.cc \
    eventql/sql/backends/csv/CSVTableScan.h \
    eventql/sql/backends/tableref.h \
    eventql/sql/defaults.cc \
    eventql/sql/defaults.h \
    eventql/sql/expressions/aggregate.cc \
    eventql/sql/expressions/aggregate.h \
    eventql/sql/expressions/boolean.cc \
    eventql/sql/expressions/boolean.h \
    eventql/sql/expressions/conversion.cc \
    eventql/sql/expressions/conversion.h \
    eventql/sql/expressions/datetime.cc \
    eventql/sql/expressions/datetime.h \
    eventql/sql/expressions/internal.cc \
    eventql/sql/expressions/internal.h \
    eventql/sql/expressions/math.cc \
    eventql/sql/expressions/math.h \
    eventql/sql/expressions/string.cc \
    eventql/sql/expressions/string.h \
    eventql/sql/expressions/table/describe_table.cc \
    eventql/sql/expressions/table/describe_table.h \
    eventql/sql/expressions/table/groupby.cc \
    eventql/sql/expressions/table/groupby.h \
    eventql/sql/expressions/table/limit.cc \
    eventql/sql/expressions/table/limit.h \
    eventql/sql/expressions/table/nested_loop_join.cc \
    eventql/sql/expressions/table/nested_loop_join.h \
    eventql/sql/expressions/table/orderby.cc \
    eventql/sql/expressions/table/orderby.h \
    eventql/sql/expressions/table/select.cc \
    eventql/sql/expressions/table/select.h \
    eventql/sql/expressions/table/show_tables.cc \
    eventql/sql/expressions/table/show_tables.h \
    eventql/sql/expressions/table/subquery.cc \
    eventql/sql/expressions/table/subquery.h \
    eventql/sql/expressions/table/tablescan.cc \
    eventql/sql/expressions/table/tablescan.h \
    eventql/sql/expressions/table_expression.h \
    eventql/sql/extensions/chartsql/areachartbuilder.cc \
    eventql/sql/extensions/chartsql/areachartbuilder.h \
    eventql/sql/extensions/chartsql/barchartbuilder.cc \
    eventql/sql/extensions/chartsql/barchartbuilder.h \
    eventql/sql/extensions/chartsql/chart_expression.cc \
    eventql/sql/extensions/chartsql/chart_expression.h \
    eventql/sql/extensions/chartsql/chartbuilder.h \
    eventql/sql/extensions/chartsql/domainconfig.cc \
    eventql/sql/extensions/chartsql/domainconfig.h \
    eventql/sql/extensions/chartsql/linechartbuilder.cc \
    eventql/sql/extensions/chartsql/linechartbuilder.h \
    eventql/sql/extensions/chartsql/pointchartbuilder.cc \
    eventql/sql/extensions/chartsql/pointchartbuilder.h \
    eventql/sql/extensions/chartsql/seriesadapter.cc \
    eventql/sql/extensions/chartsql/seriesadapter.h \
    eventql/sql/format.cc \
    eventql/sql/format.h \
    eventql/sql/parser/astnode.cc \
    eventql/sql/parser/astnode.h \
    eventql/sql/parser/astutil.cc \
    eventql/sql/parser/astutil.h \
    eventql/sql/parser/parser.cc \
    eventql/sql/parser/parser.h \
    eventql/sql/parser/token.cc \
    eventql/sql/parser/token.h \
    eventql/sql/parser/tokenize.cc \
    eventql/sql/parser/tokenize.h \
    eventql/sql/qtree/CallExpressionNode.cc \
    eventql/sql/qtree/CallExpressionNode.h \
    eventql/sql/qtree/ChartStatementNode.cc \
    eventql/sql/qtree/ChartStatementNode.h \
    eventql/sql/qtree/ColumnReferenceNode.cc \
    eventql/sql/qtree/ColumnReferenceNode.h \
    eventql/sql/qtree/DescribeTableNode.cc \
    eventql/sql/qtree/DescribeTableNode.h \
    eventql/sql/qtree/DrawStatementNode.cc \
    eventql/sql/qtree/DrawStatementNode.h \
    eventql/sql/qtree/GroupByNode.cc \
    eventql/sql/qtree/GroupByNode.h \
    eventql/sql/qtree/IfExpressionNode.cc \
    eventql/sql/qtree/IfExpressionNode.h \
    eventql/sql/qtree/JoinCondition.cc \
    eventql/sql/qtree/JoinCondition.h \
    eventql/sql/qtree/JoinNode.cc \
    eventql/sql/qtree/JoinNode.h \
    eventql/sql/qtree/LikeExpressionNode.cc \
    eventql/sql/qtree/LikeExpressionNode.h \
    eventql/sql/qtree/LimitNode.cc \
    eventql/sql/qtree/LimitNode.h \
    eventql/sql/qtree/LiteralExpressionNode.cc \
    eventql/sql/qtree/LiteralExpressionNode.h \
    eventql/sql/qtree/OrderByNode.cc \
    eventql/sql/qtree/OrderByNode.h \
    eventql/sql/qtree/QueryTreeNode.cc \
    eventql/sql/qtree/QueryTreeNode.h \
    eventql/sql/qtree/QueryTreeUtil.cc \
    eventql/sql/qtree/QueryTreeUtil.h \
    eventql/sql/qtree/QueryTreeUtil_impl.h \
    eventql/sql/qtree/RegexExpressionNode.cc \
    eventql/sql/qtree/RegexExpressionNode.h \
    eventql/sql/qtree/SelectExpressionNode.cc \
    eventql/sql/qtree/SelectExpressionNode.h \
    eventql/sql/qtree/SelectListNode.cc \
    eventql/sql/qtree/SelectListNode.h \
    eventql/sql/qtree/SequentialScanNode.cc \
    eventql/sql/qtree/SequentialScanNode.h \
    eventql/sql/qtree/ShowTablesNode.cc \
    eventql/sql/qtree/ShowTablesNode.h \
    eventql/sql/qtree/SubqueryNode.cc \
    eventql/sql/qtree/SubqueryNode.h \
    eventql/sql/qtree/TableExpressionNode.cc \
    eventql/sql/qtree/TableExpressionNode.h \
    eventql/sql/qtree/UnionNode.cc \
    eventql/sql/qtree/UnionNode.h \
    eventql/sql/qtree/ValueExpressionNode.h \
    eventql/sql/qtree/nodes/create_table.cc \
    eventql/sql/qtree/nodes/create_table.h \
    eventql/sql/qtree/qtree_coder.cc \
    eventql/sql/qtree/qtree_coder.h \
    eventql/sql/qtree/qtree_coder_impl.h \
    eventql/sql/query_plan.cc \
    eventql/sql/query_plan.h \
    eventql/sql/result_cursor.cc \
    eventql/sql/result_cursor.h \
    eventql/sql/result_list.cc \
    eventql/sql/result_list.h \
    eventql/sql/runtime/LikePattern.cc \
    eventql/sql/runtime/LikePattern.h \
    eventql/sql/runtime/QueryBuilder.cc \
    eventql/sql/runtime/QueryBuilder.h \
    eventql/sql/runtime/ScratchMemory.cc \
    eventql/sql/runtime/ScratchMemory.h \
    eventql/sql/runtime/ValueExpression.cc \
    eventql/sql/runtime/ValueExpression.h \
    eventql/sql/runtime/ValueExpressionBuilder.cc \
    eventql/sql/runtime/ValueExpressionBuilder.h \
    eventql/sql/runtime/compiler.cc \
    eventql/sql/runtime/compiler.h \
    eventql/sql/runtime/defaultruntime.cc \
    eventql/sql/runtime/defaultruntime.h \
    eventql/sql/runtime/queryplanbuilder.cc \
    eventql/sql/runtime/queryplanbuilder.h \
    eventql/sql/runtime/runtime.cc \
    eventql/sql/runtime/runtime.h \
    eventql/sql/runtime/symboltable.cc \
    eventql/sql/runtime/symboltable.h \
    eventql/sql/runtime/tablerepository.cc \
    eventql/sql/runtime/tablerepository.h \
    eventql/sql/runtime/vm.cc \
    eventql/sql/runtime/vm.h \
    eventql/sql/scheduler.cc \
    eventql/sql/scheduler.h \
    eventql/sql/scheduler/execution_context.cc \
    eventql/sql/scheduler/execution_context.h \
    eventql/sql/svalue.cc \
    eventql/sql/svalue.h \
    eventql/sql/table_iterator.h \
    eventql/sql/table_provider.h \
    eventql/sql/table_schema.cc \
    eventql/sql/table_schema.h \
    eventql/sql/transaction.cc \
    eventql/sql/transaction.h

noinst_LIBRARIES += evql_sql.a


################# LIB: evql_cstable.a ####################
evql_cstable_a_SOURCES = \
    eventql/io/cstable/CSTableReader.cc \
    eventql/io/cstable/CSTableReader.h \
    eventql/io/cstable/CSTableWriter.cc \
    eventql/io/cstable/CSTableWriter.h \
    eventql/io/cstable/LockManager.h \
    eventql/io/cstable/columns/UInt64ColumnWriter.h \
    eventql/io/cstable/columns/v1/ColumnReader.h \
    eventql/io/cstable/io/PageReader.h \
    eventql/io/cstable/io/PageWriter.h \
    eventql/io/cstable/ColumnReader.cc \
    eventql/io/cstable/ColumnReader.h \
    eventql/io/cstable/ColumnWriter.cc \
    eventql/io/cstable/ColumnWriter.h \
    eventql/io/cstable/RecordMaterializer.cc \
    eventql/io/cstable/RecordMaterializer.h \
    eventql/io/cstable/RecordShredder.cc \
    eventql/io/cstable/RecordShredder.h \
    eventql/io/cstable/TableSchema.cc \
    eventql/io/cstable/TableSchema.h \
    eventql/io/cstable/columns/BitPackedIntPageWriter.cc \
    eventql/io/cstable/columns/BitPackedIntPageWriter.h \
    eventql/io/cstable/columns/UInt64PageReader.cc \
    eventql/io/cstable/columns/UInt64PageReader.h \
    eventql/io/cstable/columns/UInt64PageWriter.cc \
    eventql/io/cstable/columns/UInt64PageWriter.h \
    eventql/io/cstable/columns/UnsignedIntColumnReader.cc \
    eventql/io/cstable/columns/UnsignedIntColumnReader.h \
    eventql/io/cstable/columns/UnsignedIntColumnWriter.cc \
    eventql/io/cstable/columns/UnsignedIntColumnWriter.h \
    eventql/io/cstable/columns/v1/BitPackedIntColumnReader.cc \
    eventql/io/cstable/columns/v1/BitPackedIntColumnReader.h \
    eventql/io/cstable/columns/v1/BitPackedIntColumnWriter.cc \
    eventql/io/cstable/columns/v1/BitPackedIntColumnWriter.h \
    eventql/io/cstable/columns/v1/BooleanColumnReader.cc \
    eventql/io/cstable/columns/v1/BooleanColumnReader.h \
    eventql/io/cstable/columns/v1/BooleanColumnWriter.cc \
    eventql/io/cstable/columns/v1/BooleanColumnWriter.h \
    eventql/io/cstable/columns/v1/ColumnWriter.cc \
    eventql/io/cstable/columns/v1/ColumnWriter.h \
    eventql/io/cstable/columns/v1/DoubleColumnReader.cc \
    eventql/io/cstable/columns/v1/DoubleColumnReader.h \
    eventql/io/cstable/columns/v1/DoubleColumnWriter.cc \
    eventql/io/cstable/columns/v1/DoubleColumnWriter.h \
    eventql/io/cstable/columns/v1/LEB128ColumnReader.cc \
    eventql/io/cstable/columns/v1/LEB128ColumnReader.h \
    eventql/io/cstable/columns/v1/LEB128ColumnWriter.cc \
    eventql/io/cstable/columns/v1/LEB128ColumnWriter.h \
    eventql/io/cstable/columns/v1/StringColumnReader.cc \
    eventql/io/cstable/columns/v1/StringColumnReader.h \
    eventql/io/cstable/columns/v1/StringColumnWriter.cc \
    eventql/io/cstable/columns/v1/StringColumnWriter.h \
    eventql/io/cstable/columns/v1/UInt32ColumnReader.cc \
    eventql/io/cstable/columns/v1/UInt32ColumnReader.h \
    eventql/io/cstable/columns/v1/UInt32ColumnWriter.cc \
    eventql/io/cstable/columns/v1/UInt32ColumnWriter.h \
    eventql/io/cstable/columns/v1/UInt64ColumnReader.cc \
    eventql/io/cstable/columns/v1/UInt64ColumnReader.h \
    eventql/io/cstable/columns/v1/UInt64ColumnWriter.cc \
    eventql/io/cstable/columns/v1/UInt64ColumnWriter.h \
    eventql/io/cstable/cstable.cc \
    eventql/io/cstable/cstable.h \
    eventql/io/cstable/io/PageIndex.cc \
    eventql/io/cstable/io/PageIndex.h \
    eventql/io/cstable/io/PageManager.cc \
    eventql/io/cstable/io/PageManager.h

noinst_LIBRARIES += evql_cstable.a


################# LIB: evql_sstable.a ####################
evql_sstable_a_SOURCES = \
    eventql/io/sstable/MetaPage.cc \
    eventql/io/sstable/MetaPage.h \
    eventql/io/sstable/RowWriter.cc \
    eventql/io/sstable/RowWriter.h \
    eventql/io/sstable/SSTableEditor_impl.h \
    eventql/io/sstable/SSTableServlet.cc \
    eventql/io/sstable/SSTableServlet.h \
    eventql/io/sstable/binaryformat.h \
    eventql/io/sstable/SSTableColumnReader.cc \
    eventql/io/sstable/SSTableColumnReader.h \
    eventql/io/sstable/SSTableColumnSchema.cc \
    eventql/io/sstable/SSTableColumnSchema.h \
    eventql/io/sstable/SSTableColumnWriter.cc \
    eventql/io/sstable/SSTableColumnWriter.h \
    eventql/io/sstable/SSTableEditor.cc \
    eventql/io/sstable/SSTableEditor.h \
    eventql/io/sstable/SSTableScan.cc \
    eventql/io/sstable/SSTableScan.h \
    eventql/io/sstable/SSTableWriter.cc \
    eventql/io/sstable/SSTableWriter.h \
    eventql/io/sstable/cursor.cc \
    eventql/io/sstable/cursor.h \
    eventql/io/sstable/fileheaderreader.cc \
    eventql/io/sstable/fileheaderreader.h \
    eventql/io/sstable/fileheaderwriter.cc \
    eventql/io/sstable/fileheaderwriter.h \
    eventql/io/sstable/index.cc \
    eventql/io/sstable/index.h \
    eventql/io/sstable/indexprovider.cc \
    eventql/io/sstable/indexprovider.h \
    eventql/io/sstable/rowoffsetindex.cc \
    eventql/io/sstable/rowoffsetindex.h \
    eventql/io/sstable/sstablereader.cc \
    eventql/io/sstable/sstablereader.h \
    eventql/io/sstable/sstablerepair.cc \
    eventql/io/sstable/sstablerepair.h

noinst_LIBRARIES += evql_sstable.a


################# LIB: evql_util.a ####################
evql_util_a_SOURCES = \
    eventql/util/CivilTime.cc \
    eventql/util/CivilTime.h \
    eventql/util/CivilTime_impl.h \
    eventql/util/HMAC.cc \
    eventql/util/HMAC.h \
    eventql/util/ISO8601.cc \
    eventql/util/ISO8601.h \
    eventql/util/InternMap.cc \
    eventql/util/InternMap.h \
    eventql/util/Language.cc \
    eventql/util/Language.h \
    eventql/util/RegExp.cc \
    eventql/util/RegExp.h \
    eventql/util/SHA1.cc \
    eventql/util/SHA1.h \
    eventql/util/StackTrace.cc \
    eventql/util/StackTrace.h \
    eventql/util/UTF8.cc \
    eventql/util/UTF8.h \
    eventql/util/UnixTime.cc \
    eventql/util/UnixTime.h \
    eventql/util/UnixTime_impl.h \
    eventql/util/VFS.cc \
    eventql/util/VFS.h \
    eventql/util/application.cc \
    eventql/util/application.h \
    eventql/util/assets.cc \
    eventql/util/assets.h \
    eventql/util/autoref.cc \
    eventql/util/autoref.h \
    eventql/util/autoref_impl.h \
    eventql/util/buffer.cc \
    eventql/util/buffer.h \
    eventql/util/bufferutil.cc \
    eventql/util/bufferutil.h \
    eventql/util/freeondestroy.h \
    eventql/util/charts/areachart.cc \
    eventql/util/charts/areachart.h \
    eventql/util/charts/axisdefinition.cc \
    eventql/util/charts/axisdefinition.h \
    eventql/util/charts/barchart.cc \
    eventql/util/charts/barchart.h \
    eventql/util/charts/canvas.cc \
    eventql/util/charts/canvas.h \
    eventql/util/charts/domain.cc \
    eventql/util/charts/domain.h \
    eventql/util/charts/domainprovider.cc \
    eventql/util/charts/domainprovider.h \
    eventql/util/charts/domainprovider_impl.h \
    eventql/util/charts/viewport.h \
    eventql/util/charts/continuousdomain.h \
    eventql/util/charts/colorpalette.h \
    eventql/util/charts/rendertarget.h \
    eventql/util/charts/discretedomain.h \
    eventql/util/charts/seriesjoin.h \
    eventql/util/charts/svgtarget.h \
    eventql/util/charts/drawable.cc \
    eventql/util/charts/drawable.h \
    eventql/util/charts/griddefinition.cc \
    eventql/util/charts/griddefinition.h \
    eventql/util/charts/legenddefinition.cc \
    eventql/util/charts/legenddefinition.h \
    eventql/util/charts/linechart.cc \
    eventql/util/charts/linechart.h \
    eventql/util/charts/pointchart.cc \
    eventql/util/charts/pointchart.h \
    eventql/util/charts/series.cc \
    eventql/util/charts/series.h \
    eventql/util/charts/timedomain.cc \
    eventql/util/charts/timedomain.h \
    eventql/util/charts/styles/style_default.h \
    eventql/util/cli/CLI.cc \
    eventql/util/cli/CLI.h \
    eventql/util/cli/CLICommand.cc \
    eventql/util/cli/CLICommand.h \
    eventql/util/cli/flagparser.cc \
    eventql/util/cli/flagparser.h \
    eventql/util/cli/term.cc \
    eventql/util/cli/term.h \
    eventql/util/csv/BinaryCSVInputStream.cc \
    eventql/util/csv/BinaryCSVInputStream.h \
    eventql/util/csv/BinaryCSVOutputStream.cc \
    eventql/util/csv/BinaryCSVOutputStream.h \
    eventql/util/csv/CSVInputStream.cc \
    eventql/util/csv/CSVInputStream.h \
    eventql/util/csv/CSVOutputStream.cc \
    eventql/util/csv/CSVOutputStream.h \
    eventql/util/defines.h \
    eventql/util/duration.cc \
    eventql/util/duration.h \
    eventql/util/duration_impl.h \
    eventql/util/exception.cc \
    eventql/util/exception.h \
    eventql/util/exceptionhandler.cc \
    eventql/util/exceptionhandler.h \
    eventql/util/fnv.cc \
    eventql/util/fnv.h \
    eventql/util/human.cc \
    eventql/util/human.h \
    eventql/util/ieee754.cc \
    eventql/util/ieee754.h \
    eventql/util/inspect.cc \
    eventql/util/inspect.h \
    eventql/util/inspect_impl.h \
    eventql/util/io/BufferedOutputStream.cc \
    eventql/util/io/BufferedOutputStream.h \
    eventql/util/io/FileLock.cc \
    eventql/util/io/FileLock.h \
    eventql/util/io/TerminalInputStream.cc \
    eventql/util/io/TerminalInputStream.h \
    eventql/util/io/TerminalOutputStream.cc \
    eventql/util/io/TerminalOutputStream.h \
    eventql/util/io/VFSFileInputStream.cc \
    eventql/util/io/VFSFileInputStream.h \
    eventql/util/io/file.cc \
    eventql/util/io/file.h \
    eventql/util/io/filerepository.cc \
    eventql/util/io/filerepository.h \
    eventql/util/io/fileutil.cc \
    eventql/util/io/fileutil.h \
    eventql/util/io/inputstream.cc \
    eventql/util/io/inputstream.h \
    eventql/util/io/mmappedfile.cc \
    eventql/util/io/mmappedfile.h \
    eventql/util/io/outputstream.cc \
    eventql/util/io/outputstream.h \
    eventql/util/io/pagemanager.cc \
    eventql/util/io/pagemanager.h \
    eventql/util/json/flatjsonreader.cc \
    eventql/util/json/flatjsonreader.h \
    eventql/util/json/json.cc \
    eventql/util/json/json.h \
    eventql/util/json/json_impl.h \
    eventql/util/json/jsontypes.h \
    eventql/util/json/jsondocument.cc \
    eventql/util/json/jsondocument.h \
    eventql/util/json/jsondocument_impl.h \
    eventql/util/json/jsoninputstream.cc \
    eventql/util/json/jsoninputstream.h \
    eventql/util/json/JSONRPCCodec.h \
    eventql/util/json/JSONRPCCodec_impl.h \
    eventql/util/json/jsonargumentlist.h \
    eventql/util/json/jsonoutputstream.cc \
    eventql/util/json/jsonoutputstream.h \
    eventql/util/json/jsonoutputstream_impl.h \
    eventql/util/json/jsonpointer.cc \
    eventql/util/json/jsonpointer.h \
    eventql/util/json/jsonrpc.cc \
    eventql/util/json/jsonrpc.h \
    eventql/util/json/jsonrpc_impl.h \
    eventql/util/json/jsonrpchttpadapter.cc \
    eventql/util/json/jsonrpchttpadapter.h \
    eventql/util/json/jsonrpcrequest.cc \
    eventql/util/json/jsonrpcrequest.h \
    eventql/util/json/jsonrpcrequest_impl.h \
    eventql/util/json/jsonrpcresponse.cc \
    eventql/util/json/jsonrpcresponse.h \
    eventql/util/json/jsonrpcresponse_impl.h \
    eventql/util/json/jsonutil.cc \
    eventql/util/json/jsonutil.h \
    eventql/util/logging.cc \
    eventql/util/logging.h \
    eventql/util/logging/loglevel.h \
    eventql/util/logging/logger.h \
    eventql/util/logging/logger_impl.h \
    eventql/util/logging/logtarget.h \
    eventql/util/logging/logoutputstream.cc \
    eventql/util/logging/logoutputstream.h \
    eventql/util/logging/syslog.cc \
    eventql/util/logging/syslog.h \
    eventql/util/net/dnscache.cc \
    eventql/util/net/dnscache.h \
    eventql/util/net/inetaddr.cc \
    eventql/util/net/inetaddr.h \
    eventql/util/net/tcpconnection.cc \
    eventql/util/net/tcpconnection.h \
    eventql/util/net/tcpserver.cc \
    eventql/util/net/tcpserver.h \
    eventql/util/net/udpserver.cc \
    eventql/util/net/udpserver.h \
    eventql/util/net/udpsocket.cc \
    eventql/util/net/udpsocket.h \
    eventql/util/option.h \
    eventql/util/option_impl.h \
    eventql/util/reflect/indexsequence.h \
    eventql/util/reflect/metaclass.h \
    eventql/util/reflect/methodcall.h \
    eventql/util/reflect/methodcall_impl.h \
    eventql/util/reflect/property.h \
    eventql/util/reflect/property_impl.h \
    eventql/util/reflect/reflect.h \
    eventql/util/traits.h \
    eventql/util/protobuf/msg.h \
    eventql/util/protobuf/DynamicMessage.cc \
    eventql/util/protobuf/DynamicMessage.h \
    eventql/util/protobuf/JSONEncoder.cc \
    eventql/util/protobuf/ProtoRef.h \
    eventql/util/protobuf/ProtoRef_impl.h \
    eventql/util/protobuf/JSONEncoder.h \
    eventql/util/protobuf/MessageDecoder.cc \
    eventql/util/protobuf/MessageDecoder.h \
    eventql/util/protobuf/MessageEncoder.cc \
    eventql/util/protobuf/MessageEncoder.h \
    eventql/util/protobuf/MessageObject.cc \
    eventql/util/protobuf/MessageObject.h \
    eventql/util/protobuf/MessagePrinter.cc \
    eventql/util/protobuf/MessagePrinter.h \
    eventql/util/protobuf/MessageSchema.cc \
    eventql/util/protobuf/MessageSchema.h \
    eventql/util/random.cc \
    eventql/util/random.h \
    eventql/util/Serializable.h \
    eventql/util/rpc/RPCContext.cc \
    eventql/util/rpc/RPCContext.h \
    eventql/util/rpc/RPCRequest.cc \
    eventql/util/rpc/RPCRequest.h \
    eventql/util/rpc/RPCService.cc \
    eventql/util/rpc/RPCService.h \
    eventql/util/rpc/RPCService_impl.h \
    eventql/util/rpc/RPCStub.cc \
    eventql/util/rpc/RPCStub.h \
    eventql/util/rpc/RPC.cc \
    eventql/util/rpc/RPCClient.cc \
    eventql/util/rpc/RPCClient.h \
    eventql/util/rpc/ServerGroup.cc \
    eventql/util/rpc/ServerGroup.h \
    eventql/util/rpc/RPC.h \
    eventql/util/rpc/RPC_impl.h \
    eventql/util/stats/counter.h \
    eventql/util/stats/counter_impl.h \
    eventql/util/stats/counterstat.h \
    eventql/util/stats/movingwindow.h \
    eventql/util/stats/multicounter.h \
    eventql/util/stats/multicounter_impl.h \
    eventql/util/stats/stat.h \
    eventql/util/stats/stats.h \
    eventql/util/stats/statsd.cc \
    eventql/util/stats/statsd.h \
    eventql/util/stats/statsdagent.cc \
    eventql/util/stats/statsdagent.h \
    eventql/util/stats/statsrepository.cc \
    eventql/util/stats/statsrepository.h \
    eventql/util/stats/statssink.cc \
    eventql/util/stats/statssink.h \
    eventql/util/status.cc \
    eventql/util/status.h \
    eventql/util/stdtypes.h \
    eventql/util/stringutil.cc \
    eventql/util/stringutil.h \
    eventql/util/hash.h \
    eventql/util/stringutil_impl.h \
    eventql/util/thread/FixedSizeThreadPool.cc \
    eventql/util/thread/FixedSizeThreadPool.h \
    eventql/util/thread/eventloop.cc \
    eventql/util/thread/eventloop.h \
    eventql/util/thread/signalhandler.cc \
    eventql/util/thread/signalhandler.h \
    eventql/util/thread/threadpool.cc \
    eventql/util/thread/threadpool.h \
    eventql/util/thread/taskscheduler.h \
    eventql/util/thread/task.h \
    eventql/util/thread/wakeup.cc \
    eventql/util/thread/wakeup.h \
    eventql/util/thread/future.h \
    eventql/util/thread/queue.h \
    eventql/util/thread/queue_impl.h \
    eventql/util/thread/CoalescingDelayedQueue.h \
    eventql/util/thread/CoalescingDelayedQueue_impl.h \
    eventql/util/thread/DelayedQueue.h \
    eventql/util/thread/DelayedQueue_impl.h \
    eventql/util/thread/future_impl.h \
    eventql/util/time_constants.h \
    eventql/util/uri.cc \
    eventql/util/uri.h \
    eventql/util/util/Base64.cc \
    eventql/util/util/Base64.h \
    eventql/util/util/BitPackDecoder.cc \
    eventql/util/util/BitPackDecoder.h \
    eventql/util/util/BitPackEncoder.cc \
    eventql/util/util/BitPackEncoder.h \
    eventql/util/util/CumulativeHistogram.cc \
    eventql/util/util/CumulativeHistogram.h \
    eventql/util/util/PersistentHashSet.cc \
    eventql/util/util/PersistentHashSet.h \
    eventql/util/util/SimpleRateLimit.cc \
    eventql/util/util/SimpleRateLimit.h \
    eventql/util/util/binarymessagereader.cc \
    eventql/util/util/binarymessagereader.h \
    eventql/util/util/binarymessagereader_impl.h \
    eventql/util/util/binarymessagewriter.cc \
    eventql/util/util/binarymessagewriter.h \
    eventql/util/RadixTree.h \
    eventql/util/RadixTree_impl.h \
    eventql/util/TimeRange.h \
    eventql/util/TimeRange_impl.h \
    eventql/util/VFSFile.h \
    eventql/util/wallclock.cc \
    eventql/util/wallclock.h \
    eventql/util/web/SecureCookie.cc \
    eventql/util/web/SecureCookie.h

    #eventql/util/MonotonicClock.cc \
    #eventql/util/MonotonicClock.h \
    #eventql/util/MonotonicTime.cc \
    #eventql/util/MonotonicTime.h \
    #eventql/util/MonotonicTime_impl.h

noinst_LIBRARIES += evql_util.a


################# LIB: evql_http.a ####################
evql_http_a_SOURCES = \
    eventql/util/http/HTTPFileDownload.cc \
    eventql/util/http/HTTPFileDownload.h \
    eventql/util/http/HTTPRequestStream.cc \
    eventql/util/http/HTTPRequestStream.h \
    eventql/util/http/HTTPResponseStream.cc \
    eventql/util/http/HTTPResponseStream.h \
    eventql/util/http/HTTPSSEParser.cc \
    eventql/util/http/httpconnectionhandler.h \
    eventql/util/http/HTTPSSEParser.h \
    eventql/util/http/HTTPSSEResponseHandler.cc \
    eventql/util/http/HTTPSSEResponseHandler.h \
    eventql/util/http/HTTPSSEStream.cc \
    eventql/util/http/HTTPSSEStream.h \
    eventql/util/http/VFSFileServlet.cc \
    eventql/util/http/VFSFileServlet.h \
    eventql/util/http/cookies.cc \
    eventql/util/http/cookies.h \
    eventql/util/http/status.h \
    eventql/util/http/httphandler.h \
    eventql/util/http/httpclient.cc \
    eventql/util/http/httpclient.h \
    eventql/util/http/httpclientconnection.cc \
    eventql/util/http/httpclientconnection.h \
    eventql/util/http/httpconnectionpool.cc \
    eventql/util/http/httpconnectionpool.h \
    eventql/util/http/httpgenerator.cc \
    eventql/util/http/httpgenerator.h \
    eventql/util/http/httpmessage.cc \
    eventql/util/http/httpmessage.h \
    eventql/util/http/httpparser.cc \
    eventql/util/http/httpparser.h \
    eventql/util/http/httprequest.cc \
    eventql/util/http/httprequest.h \
    eventql/util/http/httpresponse.cc \
    eventql/util/http/httpresponse.h \
    eventql/util/http/httpresponsefuture.cc \
    eventql/util/http/httpresponsefuture.h \
    eventql/util/http/httpresponsehandler.h \
    eventql/util/http/httprouter.cc \
    eventql/util/http/httprouter.h \
    eventql/util/http/httprouter_impl.h \
    eventql/util/http/httpserver.cc \
    eventql/util/http/httpserver.h \
    eventql/util/http/httpserverconnection.cc \
    eventql/util/http/httpserverconnection.h \
    eventql/util/http/httpstats.h \
    eventql/util/http/httpservice.cc \
    eventql/util/http/httpservice.h \
    eventql/util/http/statshttpservlet.cc \
    eventql/util/http/statshttpservlet.h

noinst_LIBRARIES += evql_http.a


################# BIN: evql ####################
evql_SOURCES = \
    eventql/cli/console.cc \
    eventql/cli/console.h \
    eventql/server/sql/codec/binary_codec.cc \
    eventql/server/sql/codec/binary_codec.h \
    eventql/evql.cc

evql_LDADD = \
    evql_http.a \
    evql_sql.a \
    evql_cstable.a \
    evql_util.a \
    $(LINENOISE_LDADD) \
    $(SIMDCOMP_LDADD) \
    $(PCRE_LDADD)

bin_PROGRAMS += evql


################# BIN: evqld ####################
evqld_SOURCES = \
    eventql/AnalyticsApp.cc \
    eventql/AnalyticsApp.h \
    eventql/AnalyticsAuth.cc \
    eventql/AnalyticsAuth.h \
    eventql/EventScanResult.cc \
    eventql/EventScanResult.h \
    eventql/JavaScriptContext.cc \
    eventql/JavaScriptContext.h \
    eventql/LogfileScanResult.cc \
    eventql/LogfileScanResult.h \
    eventql/config/namespace_config.cc \
    eventql/config/namespace_config.h \
    eventql/db/DerivedDataset.cc \
    eventql/db/DerivedDataset.h \
    eventql/db/PartitionChangeNotification.h \
    eventql/db/RecordRef.h \
    eventql/db/ServerConfig.h \
    eventql/db/TSDBTableInfo.h \
    eventql/db/TSDBTableScan.h \
    eventql/db/TSDBTableScan_impl.h \
    eventql/db/TSDBTableScanlet.h \
    eventql/db/TablePartitioner.h \
    eventql/config/config_directory.cc \
    eventql/config/config_directory.h \
    eventql/config/config_directory_legacy.cc \
    eventql/config/config_directory_legacy.h \
    eventql/config/config_directory_zookeeper.cc \
    eventql/config/config_directory_zookeeper.h \
    eventql/db/CompactionStrategy.cc \
    eventql/db/CompactionStrategy.h \
    eventql/db/CompactionWorker.cc \
    eventql/db/CompactionWorker.h \
    eventql/db/FixedShardPartitioner.cc \
    eventql/db/FixedShardPartitioner.h \
    eventql/db/LSMPartitionReader.cc \
    eventql/db/LSMPartitionReader.h \
    eventql/db/LSMPartitionReplication.cc \
    eventql/db/LSMPartitionReplication.h \
    eventql/db/LSMPartitionWriter.cc \
    eventql/db/LSMPartitionWriter.h \
    eventql/db/LSMTableIndex.cc \
    eventql/db/LSMTableIndex.h \
    eventql/db/LSMTableIndexCache.cc \
    eventql/db/LSMTableIndexCache.h \
    eventql/db/LazyPartition.cc \
    eventql/db/LazyPartition.h \
    eventql/db/LogPartitionReader.cc \
    eventql/db/LogPartitionReader.h \
    eventql/db/LogPartitionReplication.cc \
    eventql/db/LogPartitionReplication.h \
    eventql/db/LogPartitionWriter.cc \
    eventql/db/LogPartitionWriter.h \
    eventql/db/Partition.cc \
    eventql/db/Partition.h \
    eventql/db/PartitionReader.cc \
    eventql/db/PartitionReader.h \
    eventql/db/PartitionReplication.cc \
    eventql/db/PartitionReplication.h \
    eventql/db/PartitionSnapshot.cc \
    eventql/db/PartitionSnapshot.h \
    eventql/db/PartitionWriter.cc \
    eventql/db/PartitionWriter.h \
    eventql/db/RecordArena.cc \
    eventql/db/RecordArena.h \
    eventql/db/RecordSet.cc \
    eventql/db/RecordSet.h \
    eventql/db/ReplicationScheme.cc \
    eventql/db/ReplicationScheme.h \
    eventql/db/ReplicationState.cc \
    eventql/db/ReplicationState.h \
    eventql/db/ReplicationWorker.cc \
    eventql/db/ReplicationWorker.h \
    eventql/db/StaticPartitionReader.cc \
    eventql/db/StaticPartitionReader.h \
    eventql/db/StaticPartitionReplication.cc \
    eventql/db/StaticPartitionReplication.h \
    eventql/db/StaticPartitionWriter.cc \
    eventql/db/StaticPartitionWriter.h \
    eventql/db/TSDBClient.cc \
    eventql/db/TSDBClient.h \
    eventql/db/TSDBService.cc \
    eventql/db/TSDBService.h \
    eventql/db/TSDBServlet.cc \
    eventql/db/TSDBServlet.h \
    eventql/db/TSDBTableRef.cc \
    eventql/db/TSDBTableRef.h \
    eventql/db/Table.cc \
    eventql/db/Table.h \
    eventql/db/TimeWindowPartitioner.cc \
    eventql/db/TimeWindowPartitioner.h \
    eventql/db/partition_map.cc \
    eventql/db/partition_map.h \
    eventql/eventql.h \
    eventql/evqld.cc \
    eventql/mapreduce/mapreduce_preludejs.cc \
    eventql/mapreduce/MapReduceScheduler.cc \
    eventql/mapreduce/MapReduceScheduler.h \
    eventql/mapreduce/MapReduceTask.cc \
    eventql/mapreduce/MapReduceTask.h \
    eventql/mapreduce/MapReduceTaskBuilder.cc \
    eventql/mapreduce/MapReduceTaskBuilder.h \
    eventql/mapreduce/tasks/MapTableTask.cc \
    eventql/mapreduce/tasks/MapTableTask.h \
    eventql/mapreduce/tasks/ReduceTask.cc \
    eventql/mapreduce/tasks/ReduceTask.h \
    eventql/mapreduce/tasks/ReturnResultsTask.cc \
    eventql/mapreduce/tasks/ReturnResultsTask.h \
    eventql/mapreduce/tasks/SaveToTablePartitionTask.cc \
    eventql/mapreduce/tasks/SaveToTablePartitionTask.h \
    eventql/mapreduce/tasks/SaveToTableTask.cc \
    eventql/mapreduce/tasks/SaveToTableTask.h \
    eventql/server/auth/client_auth.h \
    eventql/server/auth/client_auth_trust.cc \
    eventql/server/auth/client_auth_trust.h \
    eventql/server/session.cc \
    eventql/server/session.h \
    eventql/server/server_stats.cc \
    eventql/server/server_stats.h \
    eventql/server/sql/codec/binary_codec.cc \
    eventql/server/sql/codec/binary_codec.h \
    eventql/server/sql/codec/json_codec.cc \
    eventql/server/sql/codec/json_codec.h \
    eventql/server/sql/codec/json_sse_codec.cc \
    eventql/server/sql/codec/json_sse_codec.h \
    eventql/server/sql/partition_cursor.cc \
    eventql/server/sql/partition_cursor.h \
    eventql/server/sql/pipelined_expression.cc \
    eventql/server/sql/pipelined_expression.h \
    eventql/server/sql/scheduler.cc \
    eventql/server/sql/scheduler.h \
    eventql/server/sql/sql_engine.cc \
    eventql/server/sql/sql_engine.h \
    eventql/server/sql/table_provider.cc \
    eventql/server/sql/table_provider.h \
    eventql/server/sql/table_scan.cc \
    eventql/server/sql/table_scan.h \
    eventql/server/sql/transaction_info.cc \
    eventql/server/sql/transaction_info.h \
    eventql/transport/http/EventsAPIServlet.cc \
    eventql/transport/http/EventsAPIServlet.h \
    eventql/transport/http/EventsService.cc \
    eventql/transport/http/EventsService.h \
    eventql/transport/http/LogfileAPIServlet.cc \
    eventql/transport/http/LogfileAPIServlet.h \
    eventql/transport/http/LogfileService.cc \
    eventql/transport/http/LogfileService.h \
    eventql/transport/http/MapReduceAPIServlet.cc \
    eventql/transport/http/MapReduceAPIServlet.h \
    eventql/transport/http/MapReduceService.cc \
    eventql/transport/http/MapReduceService.h \
    eventql/transport/http/api_servlet.cc \
    eventql/transport/http/api_servlet.h \
    eventql/transport/http/default_servlet.cc \
    eventql/transport/http/default_servlet.h \
    eventql/transport/http/http_auth.cc \
    eventql/transport/http/http_auth.h \
    eventql/transport/http/status_servlet.cc \
    eventql/transport/http/status_servlet.h \
    eventql/util/mdb/MDB.cc \
    eventql/util/mdb/MDB.h \
    eventql/util/mdb/MDBUtil.h \
    eventql/util/mdb/MDBUtil_impl.h \
    eventql/util/mdb/MDBCursor.cc \
    eventql/util/mdb/MDBCursor.h \
    eventql/util/mdb/MDBTransaction.cc \
    eventql/util/mdb/MDBTransaction.h

nodist_evqld_SOURCES = \
    @builddir@/eventql/AnalyticsSession.pb.cc \
    @builddir@/eventql/config/namespace_config_protos.pb.cc \
    @builddir@/eventql/db/ClusterConfig.pb.cc \
    @builddir@/eventql/db/LogPartitionCompactionState.pb.cc \
    @builddir@/eventql/db/PartitionInfo.pb.cc \
    @builddir@/eventql/db/PartitionState.pb.cc \
    @builddir@/eventql/db/RecordEnvelope.pb.cc \
    @builddir@/eventql/db/ReplicationState.pb.cc \
    @builddir@/eventql/db/TableConfig.pb.cc \
    @builddir@/eventql/db/TSDBNodeConfig.pb.cc \
    @builddir@/eventql/db/TSDBTableScanSpec.pb.cc \
    @builddir@/eventql/EventScanParams.pb.cc \
    @builddir@/eventql/LogfileScanParams.pb.cc \
    @builddir@/eventql/RemoteTSDBScanParams.pb.cc

eventql/mapreduce/mapreduce_preludejs.cc:
	echo 'eventql/mapreduce/prelude.js:${srcdir}/eventql/mapreduce/prelude.js' | ${srcdir}/eventql/util/assets.sh $(builddir)/eventql/mapreduce/mapreduce_preludejs.cc

evqld_LDADD = \
    evql_http.a \
    evql_sql.a \
    evql_cstable.a \
    evql_sstable.a \
    evql_util.a \
    $(SIMDCOMP_LDADD) \
    $(SPIDERMONKEY_LDADD) \
    $(ZOOKEEPER_LDADD) \
    $(LMDB_LDADD) \
    $(PROTOBUF_LDADD) \
    $(PCRE_LDADD)

bin_PROGRAMS += evqld


################# TEST: sql/runtime ####################
test_evql_sql_runtime_SOURCES = eventql/sql/runtime/Runtime_test.cc
test_evql_sql_runtime_LDADD = \
    evql_http.a \
    evql_sql.a \
    evql_cstable.a \
    evql_sstable.a \
    evql_util.a \
    $(SIMDCOMP_LDADD) \
    $(SPIDERMONKEY_LDADD) \
    $(LMDB_LDADD) \
    $(PROTOBUF_LDADD) \
    $(PCRE_LDADD)

TESTS += test_evql_sql_runtime
check_PROGRAMS += test_evql_sql_runtime


################# TEST: sql/qtree ####################
test_evql_sql_qtree_SOURCES = eventql/sql/qtree/QTree_test.cc
test_evql_sql_qtree_LDADD = \
    evql_http.a \
    evql_sql.a \
    evql_cstable.a \
    evql_sstable.a \
    evql_util.a \
    $(SIMDCOMP_LDADD) \
    $(SPIDERMONKEY_LDADD) \
    $(LMDB_LDADD) \
    $(PROTOBUF_LDADD) \
    $(PCRE_LDADD)

TESTS += test_evql_sql_qtree
check_PROGRAMS += test_evql_sql_qtree


################# TEST: sql/parser ####################
test_evql_sql_parser_SOURCES = eventql/sql/parser/Parser_test.cc
test_evql_sql_parser_LDADD = \
    evql_http.a \
    evql_sql.a \
    evql_cstable.a \
    evql_sstable.a \
    evql_util.a \
    $(SIMDCOMP_LDADD) \
    $(SPIDERMONKEY_LDADD) \
    $(LMDB_LDADD) \
    $(PROTOBUF_LDADD) \
    $(PCRE_LDADD)

TESTS += test_evql_sql_parser
check_PROGRAMS += test_evql_sql_parser


