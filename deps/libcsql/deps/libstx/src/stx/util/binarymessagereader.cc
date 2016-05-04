/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/util/binarymessagereader.h>

namespace stx {
namespace util {

BinaryMessageReader::BinaryMessageReader(
    void const* buf,
    size_t buf_len) :
    ptr_(buf),
    size_(buf_len),
    pos_(0) {}

}
}

