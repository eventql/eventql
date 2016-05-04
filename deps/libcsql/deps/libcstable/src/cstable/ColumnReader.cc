/**
 * This file is part of the "libcstable" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * libcstable is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <cstable/ColumnReader.h>


namespace cstable {

bool ColumnReader::readDateTime(
    uint64_t* rlvl,
    uint64_t* dlvl,
    UnixTime* value) {
  uint64_t tmp;
  if (readUnsignedInt(rlvl, dlvl, &tmp)) {
    *value = UnixTime(tmp);
    return true;
  } else {
    *value = UnixTime(0);
    return false;
  }
}

} // namespace cstable


