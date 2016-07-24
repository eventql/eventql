/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#ifndef _FNORD_SSTABLE_CURSOR_H
#define _FNORD_SSTABLE_CURSOR_H
#include <stdlib.h>
#include <string>
#include <vector>
#include <memory>
#include "eventql/util/buffer.h"


namespace sstable {

class Cursor {
public:
  Cursor();
  Cursor(const Cursor& copy) = delete;
  Cursor& operator=(const Cursor& copy) = delete;
  virtual ~Cursor();

  virtual void seekTo(size_t body_offset) = 0;
  virtual bool trySeekTo(size_t body_offset) = 0;
  virtual bool next() = 0;
  virtual bool valid() = 0;

  virtual void getKey(void** data, size_t* size) = 0;
  virtual std::string getKeyString();
  virtual Buffer getKeyBuffer();
  virtual void getData(void** data, size_t* size) = 0;
  virtual std::string getDataString();
  virtual Buffer getDataBuffer();

  virtual size_t position() const = 0;
  virtual size_t nextPosition() = 0;

};


}

#endif
