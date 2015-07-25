/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_VFS_H_
#define _STX_VFS_H_
#include <stx/VFSFile.h>
#include <stx/stdtypes.h>
#include <stx/exception.h>
#include <stx/autoref.h>

namespace stx {

class VFS {
public:
  virtual ~VFS() {}
  virtual RefPtr<VFSFile> openFile(const String& filename) = 0;
  virtual bool exists(const String& filename) = 0;
};

class WhitelistVFS : public VFS {
public:
  RefPtr<VFSFile> openFile(const String& filename) override;
  bool exists(const String& filename) override;
  void registerFile(const String vfs_path, const String& real_path);
protected:
  HashMap<String, String> whitelist_;
};

}
#endif
