/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *   - Laura Schlimmer <laura@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/sql/runtime/ResultFormat.h>
#include <eventql/util/http/HTTPResponseStream.h>
#include <eventql/util/util/binarymessagewriter.h>

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
      ScopedPtr<QueryPlan> query,
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
