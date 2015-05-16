/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_TSDB_TSDBCLIENT_H
#define _FNORD_TSDB_TSDBCLIENT_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/random.h>
#include <fnord-base/option.h>
#include <fnord-http/httpconnectionpool.h>

namespace fnord {
namespace tsdb {

class TSDBClient {
public:

  TSDBClient(
      const String& uri,
      http::HTTPConnectionPool* http);

  Vector<String> listPartitions(
      const String& stream_key,
      const DateTime& from,
      const DateTime& until);

  Buffer fetchDerivedDataset(
      const String& stream_key,
      const String& patition,
      const String& derived_dataset_name);

protected:
  String uri_;
  http::HTTPConnectionPool* http_;
};

} // namespace tdsb
} // namespace fnord

#endif
