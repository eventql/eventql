/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "logjoin/LogJoinTarget.h"

using namespace fnord;

namespace cm {

void LogJoinTarget::onSession(
    mdb::MDBTransaction* txn,
    const TrackedSession& session) {
  fnord::iputs("on session... $0", session.flushed_queries.size());
}

void LogJoinTarget::onQuery(
    mdb::MDBTransaction* txn,
    const TrackedSession& session,
    const TrackedQuery& query) {
  fnord::iputs("on query... $0", query.items.size());
}

void LogJoinTarget::onItemVisit(
    mdb::MDBTransaction* txn,
    const TrackedSession& session,
    const TrackedItemVisit& item_visit) {

}

void LogJoinTarget::onItemVisit(
    mdb::MDBTransaction* txn,
    const TrackedSession& session,
    const TrackedItemVisit& item_visit,
    const TrackedQuery& query) {

}

} // namespace cm

