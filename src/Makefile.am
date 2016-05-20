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

BUNDLED_INCLUDES = -I$(top_srcdir)/deps/3rdparty

AM_CXXFLAGS = $(BUNDLED_INCLUDES) $(LINENOISE_CLFAGS) $(PTHREAD_CFLAGS) $(PTHREAD_DEF) $(ZLIB_DEF) -std=c++0x -ftemplate-depth=500 -mno-omit-leaf-frame-pointer -fno-omit-frame-pointer -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -Wdelete-non-virtual-dtor -Wno-predefined-identifier-outside-function -O3 -g
AM_CFLAGS = $(BUNDLED_INCLUDES) $(PTHREAD_CFLAGS) $(PTHREAD_DEF) $(ZLIB_DEF) -std=c11 -mno-omit-leaf-frame-pointer -fno-omit-frame-pointer -Wall -pedantic -O3 -g
AM_LDFLAGS = $(PTHREAD_CFLAGS)

MAINTAINERCLEANFILES =   \
  Makefile.in

bin_PROGRAMS = evql
lib_LIBRARIES = libsql.a libcstable.a libsstable.a

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

libcstable_a_SOURCES = \
    eventql/io/cstable/cstable.cc \
    eventql/io/cstable/columns/BitPackedIntPageWriter.cc \
    eventql/io/cstable/columns/UInt64PageWriter.cc \
    eventql/io/cstable/columns/UInt64PageReader.cc \
    eventql/io/cstable/columns/UnsignedIntColumnWriter.cc \
    eventql/io/cstable/columns/UnsignedIntColumnReader.cc \
    eventql/io/cstable/columns/v1/BitPackedIntColumnReader.cc \
    eventql/io/cstable/columns/v1/BitPackedIntColumnWriter.cc \
    eventql/io/cstable/columns/v1/BooleanColumnReader.cc \
    eventql/io/cstable/columns/v1/BooleanColumnWriter.cc \
    eventql/io/cstable/columns/v1/ColumnWriter.cc \
    eventql/io/cstable/columns/v1/DoubleColumnReader.cc \
    eventql/io/cstable/columns/v1/DoubleColumnWriter.cc \
    eventql/io/cstable/columns/v1/LEB128ColumnReader.cc \
    eventql/io/cstable/columns/v1/LEB128ColumnWriter.cc \
    eventql/io/cstable/columns/v1/StringColumnReader.cc \
    eventql/io/cstable/columns/v1/StringColumnWriter.cc \
    eventql/io/cstable/columns/v1/UInt32ColumnReader.cc \
    eventql/io/cstable/columns/v1/UInt32ColumnWriter.cc \
    eventql/io/cstable/columns/v1/UInt64ColumnReader.cc \
    eventql/io/cstable/columns/v1/UInt64ColumnWriter.cc \
    eventql/io/cstable/io/PageManager.cc \
    eventql/io/cstable/io/PageIndex.cc \
    eventql/io/cstable/ColumnWriter.cc \
    eventql/io/cstable/ColumnReader.cc \
    eventql/io/cstable/CSTableReader.cc \
    eventql/io/cstable/CSTableWriter.cc \
    eventql/io/cstable/RecordMaterializer.cc \
    eventql/io/cstable/RecordShredder.cc \
    eventql/io/cstable/TableSchema.cc \
    eventql/io/cstable/CSTableWriter.cc

libsstable_a_SOURCES = \
    eventql/io/sstable/cursor.cc \
    eventql/io/sstable/MetaPage.cc \
    eventql/io/sstable/fileheaderreader.cc \
    eventql/io/sstable/fileheaderwriter.cc \
    eventql/io/sstable/index.cc \
    eventql/io/sstable/indexprovider.cc \
    eventql/io/sstable/rowoffsetindex.cc \
    eventql/io/sstable/RowWriter.cc \
    eventql/io/sstable/sstablereader.cc \
    eventql/io/sstable/sstablerepair.cc \
    eventql/io/sstable/SSTableEditor.cc \
    eventql/io/sstable/SSTableScan.cc \
    eventql/io/sstable/SSTableColumnSchema.cc \
    eventql/io/sstable/SSTableColumnReader.cc \
    eventql/io/sstable/SSTableColumnWriter.cc \
    eventql/io/sstable/SSTableWriter.cc

evql_util_SOURCES = \
    eventql/util/application.cc \
    eventql/util/autoref.cc \
    eventql/util/assets.cc \
    eventql/util/buffer.cc \
    eventql/util/bufferutil.cc \
    eventql/util/charts/areachart.cc \
    eventql/util/charts/axisdefinition.cc \
    eventql/util/charts/barchart.cc \
    eventql/util/charts/canvas.cc \
    eventql/util/charts/chart_test.cc \
    eventql/util/charts/domain.cc \
    eventql/util/charts/domainprovider.cc \
    eventql/util/charts/drawable.cc \
    eventql/util/charts/griddefinition.cc \
    eventql/util/charts/legenddefinition.cc \
    eventql/util/charts/linechart.cc \
    eventql/util/charts/pointchart.cc \
    eventql/util/charts/series.cc \
    eventql/util/charts/timedomain.cc \
    eventql/util/cli/flagparser.cc \
    eventql/util/cli/CLI.cc \
    eventql/util/cli/CLICommand.cc \
    eventql/util/cli/term.cc \
    eventql/util/csv/CSVOutputStream.cc \
    eventql/util/csv/BinaryCSVOutputStream.cc \
    eventql/util/csv/CSVInputStream.cc \
    eventql/util/csv/BinaryCSVInputStream.cc \
    eventql/util/UnixTime.cc \
    eventql/util/duration.cc \
    eventql/util/exception.cc \
    eventql/util/exceptionhandler.cc \
    eventql/util/fnv.cc \
    eventql/util/HMAC.cc \
    eventql/util/human.cc \
    eventql/util/ieee754.cc \
    eventql/util/inspect.cc \
    eventql/util/InternMap.cc \
    eventql/util/io/file.cc \
    eventql/util/io/FileLock.cc \
    eventql/util/io/fileutil.cc \
    eventql/util/io/filerepository.cc \
    eventql/util/io/inputstream.cc \
    eventql/util/io/outputstream.cc \
    eventql/util/io/BufferedOutputStream.cc \
    eventql/util/io/TerminalOutputStream.cc \
    eventql/util/io/TerminalInputStream.cc \
    eventql/util/io/VFSFileInputStream.cc \
    eventql/util/io/mmappedfile.cc \
    eventql/util/io/pagemanager.cc \
    eventql/util/ISO8601.cc \
    eventql/util/CivilTime.cc \
    eventql/util/Language.cc \
    eventql/util/logging.cc \
    eventql/util/logging/logoutputstream.cc \
    eventql/util/logging/syslog.cc \
    eventql/util/net/dnscache.cc \
    eventql/util/net/tcpserver.cc \
    eventql/util/net/udpserver.cc \
    eventql/util/net/udpsocket.cc \
    eventql/util/net/inetaddr.cc \
    eventql/util/net/tcpconnection.cc \
    eventql/util/random.cc \
    eventql/util/RegExp.cc \
    eventql/util/rpc/RPCRequest.cc \
    eventql/util/rpc/RPCContext.cc \
    eventql/util/rpc/RPCStub.cc \
    eventql/util/rpc/RPCService.cc \
    eventql/util/SHA1.cc \
    eventql/util/StackTrace.cc \
    eventql/util/status.cc \
    eventql/util/stats/statsdagent.cc \
    eventql/util/stats/statsrepository.cc \
    eventql/util/stats/statssink.cc \
    eventql/util/stats/statsd.cc \
    eventql/util/stringutil.cc \
    eventql/util/thread/eventloop.cc \
    eventql/util/thread/signalhandler.cc \
    eventql/util/thread/threadpool.cc \
    eventql/util/thread/FixedSizeThreadPool.cc \
    eventql/util/thread/wakeup.cc \
    eventql/util/uri.cc \
    eventql/util/UTF8.cc \
    eventql/util/util/Base64.cc \
    eventql/util/util/binarymessagereader.cc \
    eventql/util/util/binarymessagewriter.cc \
    eventql/util/util/CumulativeHistogram.cc \
    eventql/util/util/BitPackDecoder.cc \
    eventql/util/util/BitPackEncoder.cc \
    eventql/util/util/SimpleRateLimit.cc \
    eventql/util/util/PersistentHashSet.cc \
    eventql/util/VFS.cc \
    eventql/util/wallclock.cc \
    eventql/util/web/SecureCookie.cc

evql_http_SOURCES = \
    eventql/util/http/HTTPFileDownload.cc \
    eventql/util/http/HTTPRequestStream.cc \
    eventql/util/http/HTTPResponseStream.cc \
    eventql/util/http/HTTPSSEParser.cc \
    eventql/util/http/HTTPSSEResponseHandler.cc \
    eventql/util/http/HTTPSSEStream.cc \
    eventql/util/http/VFSFileServlet.cc \
    eventql/util/http/cookies.cc \
    eventql/util/http/http_test.cc \
    eventql/util/http/httpclient.cc \
    eventql/util/http/httpclientconnection.cc \
    eventql/util/http/httpconnectionpool.cc \
    eventql/util/http/httpgenerator.cc \
    eventql/util/http/httpmessage.cc \
    eventql/util/http/httpparser.cc \
    eventql/util/http/httprequest.cc \
    eventql/util/http/httpresponse.cc \
    eventql/util/http/httpresponsefuture.cc \
    eventql/util/http/httprouter.cc \
    eventql/util/http/httpserver.cc \
    eventql/util/http/httpserverconnection.cc \
    eventql/util/http/httpservice.cc \
    eventql/util/http/statshttpservlet.cc

evql_SOURCES = \
    ${evql_util_SOURCES} \
    ${evql_http_SOURCES} \
    eventql/cli/console.cc \
    eventql/server/sql/codec/binary_codec.cc \
    eventql/evql.cc

evql_LDFLAGS = \
    libsql.a \
    ${top_builddir}/deps/3rdparty/linenoise/liblinenoise.a \
    ${top_builddir}/deps/3rdparty/libsimdcomp/libsimdcomp.a
