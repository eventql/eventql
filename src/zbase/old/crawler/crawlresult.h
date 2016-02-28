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
#include <stx/UnixTime.h>
#include <stx/reflect/reflect.h>

namespace zbase {

struct CrawlResult {
  stx::UnixTime time;
  std::string url;
  std::string userdata;

  template <typename T>
  static void reflect(T* meta) {
    meta->prop(&zbase::CrawlResult::time, 1, "time", false);
    meta->prop(&zbase::CrawlResult::url, 2, "url", false);
    meta->prop(&zbase::CrawlResult::userdata, 3, "userdata", false);
  };
};

}
#endif
