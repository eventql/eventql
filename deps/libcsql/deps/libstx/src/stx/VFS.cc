/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/VFS.h>
#include <stx/io/mmappedfile.h>

namespace stx {

RefPtr<VFSFile> WhitelistVFS::openFile(const String& filename) {
  auto iter = whitelist_.find(filename);
  if (iter == whitelist_.end()) {
    RAISEF(kIndexError, "file not found in VFS: $0", filename);
  }

  return RefPtr<VFSFile>(
      new io::MmappedFile(File::openFile(iter->second, File::O_READ)));
}

bool WhitelistVFS::exists(const String& filename) {
  return whitelist_.count(filename) > 0;
}

void WhitelistVFS::registerFile(
    const String vfs_path,
    const String& real_path) {
  whitelist_[vfs_path] = real_path;
}

}
