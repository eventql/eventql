/**
 * This file is part of the "tsdb" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <eventql/util/fnv.h>
#include <eventql/util/io/fileutil.h>
#include <eventql/infra/sstable/sstablereader.h>
#include <eventql/core/PartitionReader.h>

using namespace stx;

namespace zbase {

PartitionReader::PartitionReader(
    RefPtr<PartitionSnapshot> head) :
    snap_(head) {}

} // namespace tdsb

