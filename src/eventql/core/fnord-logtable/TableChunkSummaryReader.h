/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
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
#ifndef _FNORD_LOGTABLE_TABLECHUNKSUMMARYREADER_H
#define _FNORD_LOGTABLE_TABLECHUNKSUMMARYREADER_H
#include <eventql/util/stdtypes.h>
#include <eventql/util/option.h>
#include <eventql/util/autoref.h>
#include <eventql/util/random.h>
#include <eventql/util/io/FileLock.h>
#include <eventql/util/io/mmappedfile.h>
#include <eventql/util/util/binarymessagereader.h>
#include <eventql/util/util/binarymessagewriter.h>
#include <eventql/util/protobuf/MessageSchema.h>
#include <eventql/util/protobuf/MessageObject.h>

namespace util {
namespace logtable {

class TableChunkSummaryReader {
public:

  TableChunkSummaryReader(const String& filename);

  Option<Buffer> getSummaryData(const String& summary_name) const;

  template <typename T>
  Option<T> getSummary(const String& summary_name) const;

  bool getSummaryData(
      const String& summary_name,
      void** data,
      size_t* size) const;

protected:
  HashMap<String, Pair<uint64_t, uint64_t>> offsets_;
  io::MmappedFile mmap_;
  size_t body_offset_;
};

template <typename T>
Option<T> TableChunkSummaryReader::getSummary(
    const String& summary_name) const {
  auto data = getSummaryData(summary_name);

  if (data.isEmpty()) {
    return None<T>();
  }

  util::BinaryMessageReader reader(data.get().data(), data.get().size());
  T summary;
  summary.decode(&reader);
  return Some(summary);
}


}
}
#endif
