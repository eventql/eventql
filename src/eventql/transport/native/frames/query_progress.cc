/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *   - Laura Schlimmer <laura@eventql.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include "eventql/transport/native/frames/query_progress.h"

namespace eventql {
namespace native_transport {

QueryProgressFrame::QueryProgressFrame() :
    num_rows_modified_(0),
    num_rows_scanned_(0),
    num_bytes_scanned_(0),
    query_progress_permill_(0),
    query_elapsed_ms_(0),
    query_eta_ms_(0) {}

void QueryProgressFrame::setNumRowsModified(uint64_t num_rows_modified) {
  num_rows_modified_ = num_rows_modified;
}

void QueryProgressFrame::setNumRowsScanned(uint64_t num_rows_scanned) {
  num_rows_scanned_ = num_rows_scanned;
}

void QueryProgressFrame::setNumBytesScanned(uint64_t num_bytes_scanned) {
  num_bytes_scanned_ = num_bytes_scanned;
}

void QueryProgressFrame::setQueryProgressPermill(uint64_t query_progress_permill) {
  query_progress_permill_ = query_progress_permill;
}

void QueryProgressFrame::setQueryElpasedMillis(uint64_t query_elapsed_ms) {
  query_elapsed_ms_ = query_elapsed_ms;
}

void QueryProgressFrame::setQueryETAMillis(uint64_t query_eta_ms) {
  query_eta_ms_ = query_eta_ms;
}

void QueryProgressFrame::writeToString(std::string* str) {
  //TODO
}

void QueryProgressFrame::clear() {
  num_rows_modified_ = 0;
  num_rows_scanned_ = 0;
  num_bytes_scanned_ = 0;
  query_progress_permill_ = 0;
  query_elapsed_ms_ = 0;
  query_eta_ms_ = 0;
}

} // namespace native_transport
} // namespace eventql

