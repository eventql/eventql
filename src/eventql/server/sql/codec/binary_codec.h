/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/uri.h>
#include <stx/io/file.h>
#include <stx/autoref.h>
#include <stx/http/httpmessage.h>
#include "stx/http/httprequest.h"
#include "stx/http/httpresponse.h"
#include "stx/http/httpstats.h"
#include "stx/http/httpconnectionpool.h"
#include "stx/http/httpclient.h"
#include "csql/svalue.h"
#include "csql/runtime/ExecutionContext.h"

using namespace stx;

namespace csql {

class BinaryResultParser : public stx::RefCounted {
public:

  BinaryResultParser();

  void onTableHeader(stx::Function<void (const Vector<String>& columns)> fn);
  void onRow(stx::Function<void (int argc, const SValue* argv)> fn);
  void onProgress(stx::Function<void (const ExecutionStatus& status)> fn);
  void onError(stx::Function<void (const String& error)> fn);

  void parse(const char* data, size_t size);

  bool eof() const;

protected:

  size_t parseTableHeader(const void* data, size_t size);
  size_t parseRow(const void* data, size_t size);
  size_t parseProgress(const void* data, size_t size);
  size_t parseError(const void* data, size_t size);

  stx::Buffer buf_;
  stx::Function<void (const Vector<String>& columns)> on_table_header_;
  stx::Function<void (int argc, const SValue* argv)> on_row_;
  stx::Function<void (const ExecutionStatus& status)> on_progress_;
  stx::Function<void (const String& error)> on_error_;

  bool got_header_;
  bool got_footer_;
};

}
