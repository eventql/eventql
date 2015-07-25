// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <gtest/gtest.h>
#include <cortex-base/io/MemoryFile.h>
#include <cortex-base/io/FileUtil.h>

using namespace cortex;

TEST(MemoryFile, simple) {
  const std::string path = "/foo.bar.txt";
  const std::string mimetype = "text/plain";
  const BufferRef data = "hello";
  const DateTime mtime(BufferRef("Thu, 12 Mar 2015 11:29:02 GMT"));

  MemoryFile file(path, mimetype, data, mtime);

  // printf("file.size: %zu\n", file.size());
  // printf("file.inode: %d\n", file.inode());
  // printf("file.mtime: %s\n", file.lastModified().c_str());
  // printf("file.etag: %s\n", file.etag().c_str());
  // printf("file.data: %s\n", FileUtil::read(file).c_str());

  EXPECT_TRUE(file.isRegular());
  EXPECT_TRUE(mtime.unixtime() == file.mtime());
  EXPECT_EQ(5, file.size());
  EXPECT_EQ("hello", FileUtil::read(file));
}

