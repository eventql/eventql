/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *   Copyright (c) 2015 Laura Schlimmer
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <csql/runtime/ResultFormat.h>
#include <stx/http/HTTPResponseStream.h>
#include <stx/util/binarymessagewriter.h>

/**
 * Binary SQL Query Result/Response Format
 *
 *   <response> :=
 *       <header>
 *       <event>*
 *       <footer>
 *
 *   <header> :=
 *       %0x01                      // version
 *
 *   <event> :=
 *        <table_header_event> / <row_event> / <progress_even>
 *
 *   <table_header_event>
 *       %0xf1
 *       <lenenc_int>               // number_of_columns
 *       <lenenc_string>*           // column names
 *
 *   <row_event> :=
 *       %0xf2
 *       <lenenc_int>               // number of fields
 *       <svalue>*                  // svalues
 *
 *   <progress_event> :=
 *       %0xf3
 *       <ieee754>                  // progress percent
 *
 *   <error_event> :=
 *       %0xf4
 *       <lenenc_str>               // error message
 *
 *   <footer>
 *       %0xff
 *
 */
namespace csql {

class BinaryResultFormat : public ResultFormat {
public:
  typedef Function<void (void* data, size_t size)> WriteCallback;

  BinaryResultFormat(WriteCallback write_cb);
  ~BinaryResultFormat();

  void formatResults(
      RefPtr<QueryPlan> query,
      ExecutionContext* context) override;

  void sendError(const String& error_msg);

protected:

  void sendProgress(double progress);

  void sendHeader();
  void sendFooter();

  void sendTable(
      TableExpression* stmt,
      ExecutionContext* context);

  WriteCallback write_cb_;
};

}
