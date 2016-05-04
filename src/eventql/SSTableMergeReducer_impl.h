/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
namespace zbase {

template <typename T>
SSTableMergeReducer<T>::SSTableMergeReducer(
    RefPtr<SSTableSource<T>> input,
    RefPtr<SSTableSink<T>> output) :
    ReportRDD(input.get(), output.get()),
    input_table_(input),
    output_table_(output) {}

template <typename T>
void SSTableMergeReducer<T>::onInit() {
  input_table_->forEach(std::bind(
      &SSTableMergeReducer<T>::onRow,
      this,
      std::placeholders::_1,
      std::placeholders::_2));
}

template <typename T>
void SSTableMergeReducer<T>::onRow(const String& key, const T& value) {
  counters_[key].merge(value);
}

template <typename T>
void SSTableMergeReducer<T>::onFinish() {
  for (auto& ctr : counters_) {
    output_table_->addRow(ctr.first, ctr.second);
  }
}

} // namespace zbase


