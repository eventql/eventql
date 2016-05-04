/**
 * This file is part of the "libcstable" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * libcstable is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/autoref.h>
#include <stx/io/outputstream.h>


namespace cstable {

class PageReader {
public:
  virtual void readIndex(InputStream* os) const = 0;
};

class UnsignedIntPageReader : public PageReader {
public:
  virtual uint64_t readUnsignedInt() const = 0;
};

class SignedIntPageReader : public PageReader {
public:
};

class DoublePageReader : public PageReader {
public:
};

class StringPageReader : public PageReader {
public:
};

} // namespace cstable


