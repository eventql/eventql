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
#include <assert.h>
#include "eventql/util/logging.h"
#include "eventql/sql/scheduler/aggregation_scheduler.h"

namespace eventql {

AggregationScheduler::AggregationScheduler(
    size_t max_concurrent_tasks,
    size_t max_concurrent_tasks_per_host) :
    max_concurrent_tasks_(max_concurrent_tasks),
    max_concurrent_tasks_per_host_(max_concurrent_tasks_per_host) {}

//void Aggregatio::addLocalPart(
//    const csql::GroupByNode* query);
//
void AggregationScheduler::addRemotePart(
    const csql::GroupByNode* query,
    const std::vector<std::string>& hosts) {
  assert(hosts.size() > 0);
  AggregationPart part;
  part.state = AggregationPartState::INIT;
  part.hosts = hosts;
  parts_.push_back(part);
}

void AggregationScheduler::setResultCallback(
    const std::function<void ()> fn) {

}

ReturnCode AggregationScheduler::execute() {
  for (size_t i = 0; i < max_concurrent_tasks_; ++i) {
    startNextPart();
  }
  return ReturnCode::success();
}

void AggregationScheduler::startNextPart() {
  auto iter = parts_.begin();
  while (iter != parts_.end()) {
    switch (iter->state) {
      case AggregationPartState::INIT:
      case AggregationPartState::RETRY:
        break;

      case AggregationPartState::RUNNING:
      case AggregationPartState::DONE:
        ++iter;
        continue;
    }

    break;
  }

  if (iter == parts_.end()) {
    return;
  }

  startConnection(&*iter);
}

void AggregationScheduler::startConnection(AggregationPart* part) {
  Connection connection;
  connection.state = ConnectionState::INIT;
  connection.host = part->hosts[0];
  connections_.push_back(connection);

  part->state = AggregationPartState::RUNNING;

  logDebug("evql", "Opening connection to $0", connection.host);
}


//void sendFrame();
//void recvFrame();

} // namespace eventql


