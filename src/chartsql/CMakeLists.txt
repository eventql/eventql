# This file is part of the "libfnord" project
#   Copyright (c) 2015 Paul Asmuth
#
# FnordMetric is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License v3.0. You should have received a
# copy of the GNU General Public License along with this program. If not, see
# <http://www.gnu.org/licenses/>.
add_library(chartsql-lib OBJECT
    svalue.cc
    Expression.cc
    format.cc
    areachartbuilder.cc
    barchartbuilder.cc
    domainconfig.cc
    drawstatement.cc
    linechartbuilder.cc
    pointchartbuilder.cc
    #queryendpoint.cc
    query.cc
    #queryservice.cc
    seriesadapter.cc
    CSTableScan.cc
    backends/csv/csvbackend.cc
    backends/csv/csvinputstream.cc
    backends/csv/csvtableref.cc
    backends/mysql/mysqlbackend.cc
    backends/mysql/mysqlconnection.cc
    backends/mysql/mysqltableref.cc
    backends/metricservice/metrictableref.cc
    backends/metricservice/metrictablerepository.cc
    expressions/aggregate.cc
    expressions/boolean.cc
    expressions/datetime.cc
    expressions/math.cc
    parser/astnode.cc
    parser/astutil.cc
    parser/parser.cc
    parser/token.cc
    parser/tokenize.cc
    qtree/BuiltinExpressionNode.cc
    qtree/FieldReferenceNode.cc
    qtree/SelectProjectAggregateNode.cc
    runtime/CompiledProgram.cc
    runtime/runtime.cc
    runtime/symboltable.cc
    runtime/queryplannode.cc
    runtime/compile.cc
    runtime/defaultruntime.cc
    runtime/importstatement.cc
    runtime/tablerepository.cc
    runtime/queryplanbuilder.cc
    runtime/queryplan.cc
    runtime/tablescan.cc
    runtime/orderby.cc
    runtime/groupovertimewindow.cc
    runtime/execute.cc)
