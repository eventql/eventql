/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Laura Schlimmer <laura@eventql.io>
 *   - Paul Asmuth <paul@eventql.io>
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
#include "eventql/sql/qtree/GroupByNode.h"
#include "eventql/util/return_code.h"

namespace eventql {

class AggregationScheduler {
public:

  void addLocalPart(
      const csql::GroupByNode* query);

  void addRemotePart(
      const csql::GroupByNode* query,
      const std::vector<std::string>& hosts);

  void setResultCallback(
      const std::function<void ()> fn);

  ReturnCode execute();

protected:

  void sendFrame();
  void recvFrame();

  enum AggregationPartState {
    INIT, RUNNING, RETRY, DONE
  };

  struct AggregationPart {
    AggregationPartState state;
  };

  struct ConnectionState {
  };

  std::deque<AggregationPart> parts_;
  bool done_;
};

} // namespace eventql

