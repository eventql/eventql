/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include "eventql/eventql.h"
#include <eventql/util/uri.h>
#include <eventql/util/io/file.h>
#include <eventql/util/autoref.h>
#include <eventql/util/http/httpmessage.h>
#include "eventql/util/http/httprequest.h"
#include "eventql/util/http/httpresponse.h"
#include "eventql/util/http/httpstats.h"
#include "eventql/util/http/httpconnectionpool.h"
#include "eventql/util/http/httpclient.h"
#include "eventql/sql/svalue.h"
#include "eventql/sql/query_plan.h"

namespace csql {


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

struct ExecutionStatus {
  double progress;
  size_t num_subtasks_total;
  size_t num_subtasks_completed;
  size_t num_rows_scanned;
};


class BinaryResultParser : public RefCounted {
public:

  BinaryResultParser();

  void onTableHeader(Function<void (const Vector<String>& columns)> fn);
  void onRow(Function<void (int argc, const SValue* argv)> fn);
  void onProgress(Function<void (const ExecutionStatus& status)> fn);
  void onError(Function<void (const String& error)> fn);

  void parse(const char* data, size_t size);

  bool eof() const;

protected:

  size_t parseTableHeader(const void* data, size_t size);
  size_t parseRow(const void* data, size_t size);
  size_t parseProgress(const void* data, size_t size);
  size_t parseError(const void* data, size_t size);

  Buffer buf_;
  Function<void (const Vector<String>& columns)> on_table_header_;
  Function<void (int argc, const SValue* argv)> on_row_;
  Function<void (const ExecutionStatus& status)> on_progress_;
  Function<void (const String& error)> on_error_;

  bool got_header_;
  bool got_footer_;
};

class BinaryResultFormat  {
public:
  typedef Function<void (void* data, size_t size)> WriteCallback;

  BinaryResultFormat(WriteCallback write_cb, bool truncate_rows = false);
  ~BinaryResultFormat();

  //void formatResults(
  //    QueryPlan* query,
  //    ExecutionContext* context) override;

  void sendResults(QueryPlan* query);
  void sendError(const String& error_msg);
  void sendProgress(double progress);

protected:

  void sendTable(QueryPlan* query, size_t stmt_idx);

  void sendHeader();
  void sendFooter();

  WriteCallback write_cb_;
  bool truncate_rows_;
};


}
