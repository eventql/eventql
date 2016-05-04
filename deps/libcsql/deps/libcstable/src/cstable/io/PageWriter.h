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

class PageWriter {
public:
  virtual void writeIndex(OutputStream* os) const = 0;
};

class UnsignedIntPageWriter : public PageWriter {
public:
  virtual void appendValue(uint64_t value) = 0;
};

class SignedIntPageWriter : public PageWriter {
public:
  virtual void appendValue(int64_t value) = 0;
};

class DoublePageWriter : public PageWriter {
public:
  virtual void appendValue(double value) = 0;
};

class StringPageWriter : public PageWriter {
public:
  void appendValue(const String& value);
  virtual void appendValue(const char* data, size_t size) = 0;
};

} // namespace cstable


