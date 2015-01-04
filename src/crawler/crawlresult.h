/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_CRAWLRESEULT_H
#define _CM_CRAWLRESEULT_H
#include <stdlib.h>
#include <string>
#include <fnord/base/datetime.h>
#include <fnord/reflect/reflect.h>

namespace cm {

struct CrawlResult {
  fnord::DateTime time;
  std::string url;
  std::string userdata;

  template <typename T>
  static void reflect(T* meta) {
    meta->prop(&cm::CrawlResult::time, 1, "time", false);
    meta->prop(&cm::CrawlResult::url, 2, "url", false);
    meta->prop(&cm::CrawlResult::userdata, 3, "userdata", false);
  };
};

}
#endif
