// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/io/FileUtil.h>
#include <cortex-base/io/PageManager.h>
#include <cortex-base/io/FileDescriptor.h>
#include <gtest/gtest.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/fcntl.h>
#include <unistd.h>

using namespace cortex;
using namespace cortex::io;

class ConcreteTestPageManager : public PageManager {
public:
  ConcreteTestPageManager() : PageManager(4096) {}
  std::unique_ptr<PageRef> getPage(const PageManager::Page& page) override {
    return std::unique_ptr<PageRef>(nullptr);
  }
  size_t endPos() const { return end_pos_; }
};

class TestMmapPageManager : public MmapPageManager {
public:
  explicit TestMmapPageManager(
      const std::string& filename,
      size_t len) :
      MmapPageManager(filename, len) {}

  RefPtr<MmappedFile> getMmappedFileTest(uint64_t last_byte) {
    getPage(PageManager::Page(0, last_byte));
    return getMmappedFile(last_byte);
  }
};

class PageManagerTest : public testing::Test {
 public:
  void SetUp() override {
    FileUtil::mkdir_p("build/tests/tmp");
  }

  void TearDown() override {
  }
};

TEST_F(PageManagerTest, TestAbstractPageManagerAlloc) {
    ConcreteTestPageManager page_manager;

    auto page1 = page_manager.allocPage(3000);
    EXPECT_EQ(page_manager.endPos(), 4096);
    EXPECT_EQ(page1.offset, 0);
    EXPECT_EQ(page1.size, 4096);
    page_manager.freePage(page1);
}

TEST_F(PageManagerTest, TestAbstractPageManagerAllocMulti) {
    ConcreteTestPageManager page_manager;

    auto page1 = page_manager.allocPage(3000);
    EXPECT_EQ(page_manager.endPos(), 4096);
    EXPECT_EQ(page1.offset, 0);
    EXPECT_EQ(page1.size, 4096);
    page_manager.freePage(page1);

    auto page2 = page_manager.allocPage(8000);
    EXPECT_EQ(page_manager.endPos(), 12288);
    EXPECT_EQ(page2.offset, 4096);
    EXPECT_EQ(page2.size, 8192);

    auto page3 = page_manager.allocPage(3000);
    EXPECT_EQ(page3.offset, 0);
    EXPECT_EQ(page3.size, 4096);
}

TEST_F(PageManagerTest, TestAbstractPageManagerAllocFree) {
    ConcreteTestPageManager page_manager;

    auto page1 = page_manager.allocPage(3000);
    EXPECT_EQ(page_manager.endPos(), 4096);
    EXPECT_EQ(page1.offset, 0);
    EXPECT_EQ(page1.size, 4096);
    page_manager.freePage(page1);

    auto page2 = page_manager.allocPage(8000);
    EXPECT_EQ(page_manager.endPos(), 12288);
    EXPECT_EQ(page2.offset, 4096);
    EXPECT_EQ(page2.size, 8192);

    auto page3 = page_manager.allocPage(3000);
    EXPECT_EQ(page3.offset, 0);
    EXPECT_EQ(page3.size, 4096);
    page_manager.freePage(page2);

    auto page4 = page_manager.allocPage(4000);
    EXPECT_EQ(page_manager.endPos(), 12288);
    EXPECT_EQ(page4.offset, 4096);
    EXPECT_EQ(page4.size, 8192);
}

TEST_F(PageManagerTest, TestMmapPageManager) {
  FileDescriptor fd = open("build/tests/tmp/__fnordmetric_testMmapPageManager",
      O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR);
  EXPECT_TRUE(fd > 0);
  auto page_manager = new TestMmapPageManager(
      "build/tests/tmp/__fnordmetric_testMmapPageManager", 0);

  auto mfile1 = page_manager->getMmappedFileTest(3000);
  auto mfile2 = page_manager->getMmappedFileTest(304200);
  auto page_size = sysconf(_SC_PAGESIZE);
  EXPECT_EQ(mfile1->size, page_size * MmapPageManager::kMmapSizeMultiplier);
  EXPECT_EQ((void *) mfile1.get(), (void *) mfile2.get());
  mfile2->ref();

  auto mfile3 = page_manager->getMmappedFileTest(
      page_size * MmapPageManager::kMmapSizeMultiplier + 1);
  EXPECT_EQ(mfile3->size, page_size * MmapPageManager::kMmapSizeMultiplier * 2);
  EXPECT_TRUE(mfile3 != mfile2);
  mfile2->unref();

  delete page_manager;
  unlink("build/tests/tmp/__fnordmetric_testMmapPageManager");
}

