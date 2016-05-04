/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_CSVSINK_H
#define _CM_CSVSINK_H
#include "eventql/Report.h"
#include "eventql/util/stdtypes.h"
#include "eventql/util/io/file.h"
#include "eventql/util/io/mmappedfile.h"

using namespace stx;

namespace zbase {

class CSVSink : public ReportSink {
public:

  CSVSink(
      const String& tempdir = "/tmp");

  void open();
  void addRow(const Vector<String>& row);
  RefPtr<VFSFile> finalize();

  String contentType() const override {
    return "application/csv; charset=utf-8";
  }

protected:
  String filename_;
  File file_;
};

} // namespace zbase

#endif
