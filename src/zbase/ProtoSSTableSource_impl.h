/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include <stx/logging.h>

namespace zbase {

template <typename T>
AbstractProtoSSTableSource<T>::AbstractProtoSSTableSource() :
    filter_(None<String>()) {}

template <typename T>
void AbstractProtoSSTableSource<T>::read(dproc::TaskContext* context) {
  size_t row_idx = 0;
  auto num_deps = context->numDependencies();
  for (size_t i = 0; i < num_deps; ++i) {
    try {
      sstable::SSTableReader reader(context->getDependency(i)->getData());
      if (!reader.isFinalized()) {
        RAISEF(kRuntimeError, "unfinished sstable: $0", i);
      }

      if (reader.bodySize() == 0) {
        stx::logWarning(
            "cm.reportbuild",
            "Warning: empty sstable: $0",
            i);

        continue;
      }

      /* get sstable cursor */
      auto cursor = reader.getCursor();
      auto body_size = reader.bodySize();

      /* read sstable rows */
      for (; cursor->valid(); ++row_idx) {
        auto key = cursor->getKeyString();
        auto data = cursor->getDataBuffer();

        if (filter_.isEmpty() ||
            StringUtil::beginsWith(key, filter_.get())) {
          T val;
          msg::decode<T>(data.data(), data.size(), &val);

          for (const auto& cb : callbacks_) {
            cb(key, val);
          }
        }

        if (!cursor->next()) {
          break;
        }
      }
    } catch (const std::exception& e) {
      stx::logError(
          "cm.reportbuild",
          e,
          "error while reading sstable: $0",
          i);

      throw;
    }
  }
}

template <typename T>
void AbstractProtoSSTableSource<T>::forEach(CallbackFn fn) {
  callbacks_.emplace_back(fn);
}

template <typename T>
void AbstractProtoSSTableSource<T>::setKeyPrefixFilter(const String& prefix) {
  filter_ = Some(prefix);
}

template <typename T>
ProtoSSTableSource<T>::ProtoSSTableSource(
    const List<dproc::TaskDependency>& deps) :
    deps_(deps) {}

template <typename T>
List<dproc::TaskDependency> ProtoSSTableSource<T>::dependencies() const {
  return deps_;
}

} // namespace zbase

