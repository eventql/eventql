/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
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
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/util/io/outputstream.h>


namespace cstable {

class PageReader {
public:
  virtual ~PageReader() = default;
  virtual void rewind() = 0;
};

class UnsignedIntPageReader : public PageReader {
public:
  virtual uint64_t readUnsignedInt() = 0;
  virtual uint64_t peek() = 0;
  virtual bool eofReached() = 0;
};

class SignedIntPageReader : public PageReader {
public:
};

class FloatPageReader : public PageReader {
public:
  virtual double readFloat() = 0;
};

class StringPageReader : public PageReader {
public:
  virtual void readString(String* value) = 0;
};

} // namespace cstable


