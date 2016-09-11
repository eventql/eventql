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
#pragma once
#include "eventql/eventql.h"
#include "eventql/util/io/outputstream.h"

namespace eventql {
namespace native_transport {

class QueryProgressFrame {
public:

  QueryProgressFrame();

  void setNumRowsModified(uint64_t num_rows_modified);
  void setNumRowsScanned(uint64_t num_rows_scanned);
  void setNumBytesScanned(uint64_t num_bytes_scanned);
  void setQueryProgressPermill(uint64_t query_progress_permill);
  void setQueryElapsedMillis(uint64_t query_elapsed_ms);
  void setQueryETAMillis(uint64_t query_eta_ms);

  void writeTo(OutputStream* os);
  void clear();

protected:
  uint64_t num_rows_modified_;
  uint64_t num_rows_scanned_;
  uint64_t num_bytes_scanned_;
  uint64_t query_progress_permill_;
  uint64_t query_elapsed_ms_;
  uint64_t query_eta_ms_;
};

} // namespace native_transport
} // namespace eventql

