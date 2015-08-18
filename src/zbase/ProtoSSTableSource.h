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
#include "sstable/sstablereader.h"
#include "sstable/SSTableEditor.h"

using namespace stx;

namespace cm {

template <typename T>
class AbstractProtoSSTableSource : public ReportSource {
public:
  typedef Function<void (const String&, const T&)> CallbackFn;

  AbstractProtoSSTableSource();

  void forEach(CallbackFn fn);

  void read(dproc::TaskContext* context);

  void setKeyPrefixFilter(const String& key_prefix);

protected:
  List<CallbackFn> callbacks_;
  Option<String> filter_;
};

template <typename T>
class ProtoSSTableSource : public AbstractProtoSSTableSource<T> {
public:
  ProtoSSTableSource(const List<dproc::TaskDependency>& deps);
  List<dproc::TaskDependency> dependencies() const;
protected:
  List<dproc::TaskDependency> deps_;
};

} // namespace cm

#include "ProtoSSTableSource_impl.h"
