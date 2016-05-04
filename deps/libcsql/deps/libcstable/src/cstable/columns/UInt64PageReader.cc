/**
 * This file is part of the "libcstable" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * libcstable is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <cstable/columns/UInt64PageReader.h>


namespace cstable {

UInt64PageReader::UInt64PageReader(
    RefPtr<PageManager> page_mgr) :
    page_mgr_(page_mgr) {

}

uint64_t UInt64PageReader::readUnsignedInt() const {
  return 0;
}

void UInt64PageReader::readIndex(InputStream* os) const {

}

} // namespace cstable


