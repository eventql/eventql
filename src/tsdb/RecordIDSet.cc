/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <tsdb/RecordIDSet.h>
#include <stx/io/fileutil.h>

using namespace stx;

namespace tsdb {

RecordIDSet::RecordIDSet(
    const String& filepath) :
    fpath_(filepath),
    nslots_(0) {
  if (FileUtil::exists(filepath)) {
    reopenFile();
  }
}

//void RecordIDSet::addRecordID(const SHA1Hash& record_id);
//bool RecordIDSet::hasRecordID(const SHA1Hash& record_id);

//Set<SHA1Hash> RecordIDSet::fetchRecordIDs();

void RecordIDSet::reopenFile() {

}

} // namespace tdsb

