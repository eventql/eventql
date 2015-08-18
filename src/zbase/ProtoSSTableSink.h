/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include "zbase/Report.h"

#include <inventory/ItemRef.h>
#include "sstable/sstablereader.h"
#include "sstable/SSTableEditor.h"

using namespace stx;

namespace cm {

template <typename T>
class ProtoSSTableSink : public ReportSink {
public:

  ProtoSSTableSink(const String& tempdir = "/tmp");

  void open();
  void addRow(const String& key, const T& value);
  RefPtr<VFSFile> finalize();

  String contentType() const override {
    return "application/octet-stream";
  }

protected:
  String output_file_;
  std::unique_ptr<sstable::SSTableEditor> sstable_writer_;
};

} // namespace cm

#include "ProtoSSTableSink_impl.h"
