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
ProtoSSTableMergeReducer<T>::ProtoSSTableMergeReducer(
    RefPtr<ProtoSSTableSource<T>> input,
    RefPtr<ProtoSSTableSink<T>> output,
    Function<void (T*, const T&)> merge_fn) :
    ReportRDD(input.get(), output.get()),
    input_table_(input),
    output_table_(output),
    merge_(merge_fn) {}

template <typename T>
void ProtoSSTableMergeReducer<T>::onInit() {
  input_table_->forEach(std::bind(
      &ProtoSSTableMergeReducer<T>::onRow,
      this,
      std::placeholders::_1,
      std::placeholders::_2));
}

template <typename T>
void ProtoSSTableMergeReducer<T>::onRow(const String& key, const T& value) {
  merge_(&map_[key], value);
}

template <typename T>
void ProtoSSTableMergeReducer<T>::onFinish() {
  for (auto& ctr : map_) {
    output_table_->addRow(ctr.first, ctr.second);
  }
}

} // namespace zbase


