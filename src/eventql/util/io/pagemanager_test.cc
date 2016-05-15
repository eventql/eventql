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
#include <io/fileutil.h>
#include <io/pagemanager.h>
#include <util/unittest.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/fcntl.h>
#include <unistd.h>

#include "eventql/eventql.h"

UNIT_TEST(PageManagerTest);

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

  MmappedFile* getMmappedFileTest(uint64_t last_byte) {
    getPage(PageManager::Page(0, last_byte));
    return getMmappedFile(last_byte);
  }
};

TEST_INITIALIZER(PageManagerTest, SetupTempFolder, [] () {
  FileUtil::mkdir_p("build/tests/tmp");
});

TEST_CASE(PageManagerTest, TestAbstractPageManagerAlloc, [] () {
    ConcreteTestPageManager page_manager;

    auto page1 = page_manager.allocPage(3000);
    EXPECT_EQ(page_manager.endPos(), 4096);
    EXPECT_EQ(page1.offset, 0);
    EXPECT_EQ(page1.size, 4096);
    page_manager.freePage(page1);
});

TEST_CASE(PageManagerTest, TestAbstractPageManagerAllocMulti, [] () {
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
});

TEST_CASE(PageManagerTest, TestAbstractPageManagerAllocFree, [] () {
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
});

TEST_CASE(PageManagerTest, TestMmapPageManager, [] () {
  int fd = open("build/tests/tmp/__libstx_testMmapPageManager",
      O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR);
  EXPECT(fd > 0);
  auto page_manager = new TestMmapPageManager(
      "build/tests/tmp/__libstx_testMmapPageManager", 0);

  auto mfile1 = page_manager->getMmappedFileTest(3000);
  auto mfile2 = page_manager->getMmappedFileTest(304200);
  auto page_size = sysconf(_SC_PAGESIZE);
  EXPECT_EQ(mfile1->size, page_size * MmapPageManager::kMmapSizeMultiplier);
  EXPECT_EQ((void *) mfile1, (void *) mfile2);
  mfile2->incrRefs();

  auto mfile3 = page_manager->getMmappedFileTest(
      page_size * MmapPageManager::kMmapSizeMultiplier + 1);
  EXPECT_EQ(mfile3->size, page_size * MmapPageManager::kMmapSizeMultiplier * 2);
  EXPECT(mfile3 != mfile2);
  mfile2->decrRefs();

  delete page_manager;
  unlink("build/tests/tmp/__libstx_testMmapPageManager");
  close(fd);
});

