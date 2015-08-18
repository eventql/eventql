/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
namespace cm {

template <typename T>
SSTableSource<T>::SSTableSource(
    const List<dproc::TaskDependency>& deps) :
    deps_(deps) {}

template <typename T>
void SSTableSource<T>::read(dproc::TaskContext* context) {
  size_t row_idx = 0;
  for (size_t i = 0; i < deps_.size(); ++i) {
    try {
      sstable::SSTableReader reader(context->getDependency(i)->encode());
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

        util::BinaryMessageReader reader(data.data(), data.size());

        T val;
        val.decode(&reader);

        for (const auto& cb : callbacks_) {
          cb(key, val);
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
void SSTableSource<T>::forEach(CallbackFn fn) {
  callbacks_.emplace_back(fn);
}

template <typename T>
List<dproc::TaskDependency> SSTableSource<T>::dependencies() const {
  return deps_;
}

} // namespace cm

