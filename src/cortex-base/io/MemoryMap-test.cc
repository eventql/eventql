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
#include <cortex-base/io/MemoryMap.h>
#include <cortex-base/io/FileUtil.h>
#include <cortex-base/io/FileDescriptor.h>
#include <fcntl.h>

using namespace cortex;

TEST(MemoryMap, rdonly) {
  const std::string path = "/foo.bar.txt";
  const std::string mimetype = "text/plain";
  const BufferRef data = "hello";
  const DateTime mtime(BufferRef("Thu, 12 Mar 2015 11:29:02 GMT"));

  MemoryFile file(path, mimetype, data, mtime);

  std::unique_ptr<MemoryMap> mm = file.createMemoryMap(false);

  ASSERT_TRUE(mm.get() != nullptr);
  EXPECT_TRUE(mm->isReadable());
  EXPECT_FALSE(mm->isWritable());
  ASSERT_EQ(5, mm->size());

  const char* mmbuf = (const char*) mm->data();
  EXPECT_EQ('h', mmbuf[0]);
  EXPECT_EQ('e', mmbuf[1]);
  EXPECT_EQ('l', mmbuf[2]);
  EXPECT_EQ('l', mmbuf[3]);
  EXPECT_EQ('o', mmbuf[4]);
}

TEST(MemoryMap, readAndWritable) {
  const std::string path = "/foo.bar.txt";
  const std::string mimetype = "text/plain";
  const BufferRef data = "hello";
  const DateTime mtime(BufferRef("Thu, 12 Mar 2015 11:29:02 GMT"));

  MemoryFile file(path, mimetype, data, mtime);

  { // mutate and verify file contents
    std::unique_ptr<MemoryMap> mm = file.createMemoryMap(true);
    ASSERT_TRUE(mm.get() != nullptr);
    ASSERT_EQ(5, mm->size());
    EXPECT_TRUE(mm->isReadable());
    EXPECT_TRUE(mm->isWritable());

    char* in = (char*) mm->data();
    in[0] = 'a';
    in[1] = 'b';
    in[2] = 'c';
    in[3] = 'd';
    in[4] = 'e';
  }

  { // readout and verify file contents
    FileDescriptor fd = file.createPosixChannel(File::Read);
    Buffer out = FileUtil::read(fd);

    EXPECT_EQ('a', out[0]);
    EXPECT_EQ('b', out[1]);
    EXPECT_EQ('c', out[2]);
    EXPECT_EQ('d', out[3]);
    EXPECT_EQ('e', out[4]);
  }
}
