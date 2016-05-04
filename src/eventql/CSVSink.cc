/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <stx/json/json.h>
#include <stx/io/fileutil.h>
#include <stx/random.h>
#include "eventql/CSVSink.h"

using namespace stx;

namespace zbase {

CSVSink::CSVSink(
    const String& tempdir) :
    filename_(FileUtil::joinPaths(tempdir, Random::singleton()->hex64())),
    file_(File::openFile(filename_, File::O_CREATEOROPEN | File::O_WRITE)) {}

void CSVSink::open() {}

void CSVSink::addRow(const Vector<String>& row) {
  auto col_sep = ";";
  auto row_sep = "\n";

  Buffer buf;
  for (const auto& c : row) {
    buf.append(c.data(), c.size());
    buf.append(col_sep);
  }

  buf.append(row_sep);
  file_.write(buf.data(), buf.size());
}

RefPtr<VFSFile> CSVSink::finalize() {
  return new io::MmappedFile(
      File::openFile(filename_, File::O_READ | File::O_AUTODELETE));
}

} // namespace zbase

