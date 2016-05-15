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
#ifndef _FNORD_LOGTABLE_REMOTETABLEREADER_H
#define _FNORD_LOGTABLE_REMOTETABLEREADER_H
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/util/http/httpconnectionpool.h>
#include <fnord-logtable/AbstractTableReader.h>

namespace util {
namespace logtable {

class RemoteTableReader : public AbstractTableReader{
public:

  RemoteTableReader(
      const String& table_name,
      const msg::MessageSchema& schema,
      const URI& uri,
      http::HTTPConnectionPool* http);

  const String& name() const override;
  const msg::MessageSchema& schema() const override;

  RefPtr<TableSnapshot> getSnapshot() override;

  size_t fetchRecords(
      const String& replica,
      uint64_t start_sequence,
      size_t limit,
      Function<bool (const msg::MessageObject& record)> fn) override;

protected:
  String name_;
  msg::MessageSchema schema_;
  URI uri_;
  http::HTTPConnectionPool* http_;
};

} // namespace logtable
} // namespace util

#endif

