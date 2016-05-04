/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_JSONSINK_H
#define _CM_JSONSINK_H
#include "eventql/Report.h"
#include "stx/stdtypes.h"
#include "stx/io/file.h"
#include "stx/io/mmappedfile.h"
#include "stx/json/json.h"

using namespace stx;

namespace zbase {

class JSONSink : public ReportSink {
public:

  JSONSink();

  void open();
  RefPtr<VFSFile> finalize();

  json::JSONOutputStream* json();

  String contentType() const override {
    return "application/json; charset=utf-8";
  }

protected:
  BufferRef buf_;
  json::JSONOutputStream json_;
};

} // namespace zbase

#endif
