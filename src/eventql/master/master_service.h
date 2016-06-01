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
#pragma once
#include "eventql/eventql.h"
#include "eventql/config/config_directory.h"
#include <eventql/util/stdtypes.h>
#include <eventql/util/status.h>

namespace eventql {

class MasterService {
public:

  MasterService(ConfigDirectory* cdir);

  Status runOnce();

protected:

  Status rebalanceTable(TableDefinition tbl_cfg);

  String pickMetadataServer() const;
  String pickServer() const;

  ConfigDirectory* cdir_;
  size_t replication_factor_;
  size_t metadata_replication_factor_;
  Set<String> all_servers_;
  Vector<String> live_servers_;

};

} // namespace eventql

