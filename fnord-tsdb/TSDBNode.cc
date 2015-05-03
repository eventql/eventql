/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-tsdb/TSDBNode.h>

namespace fnord {
namespace tsdb {

TSDBNode::TSDBNode(
    const String& nodeid,
    const String& db_path) :
    nodeid_(nodeid),
    db_path_(db_path),
    db_(
        mdb::MDB::open(
            db_path_,
            false,
            1024 * 1024 * 1024, // 1 GiB
            nodeid + ".db",
            nodeid + ".db.lck"
            )) {}

} // namespace tdsb
} // namespace fnord

