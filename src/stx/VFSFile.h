/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_VFSFILE_H_
#define _STX_VFSFILE_H_
#include <stx/stdtypes.h>
#include <stx/autoref.h>

namespace stx {

class VFSFile : public RefCounted {
public:
  virtual ~VFSFile() {}

  virtual size_t size() const = 0;
  virtual void* data() const = 0;

  template <typename T>
  inline T* structAt(size_t pos) const {
    return (T*) (((char *) data()) + pos);
  }

};

}
#endif
