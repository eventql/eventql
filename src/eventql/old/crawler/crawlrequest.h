/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_CRAWLREQUEST_H
#define _CM_CRAWLREQUEST_H
#include <stdlib.h>
#include <string>
#include <eventql/util/UnixTime.h>
#include <eventql/util/reflect/reflect.h>

namespace zbase {

struct CrawlRequest {
  CrawlRequest() : follow_redirects(false) {}

  std::string url;
  std::string target_feed;
  std::string userdata;
  bool follow_redirects;

  template <typename T>
  static void reflect(T* meta) {
    meta->prop(&zbase::CrawlRequest::url, 1, "url", false);
    meta->prop(&zbase::CrawlRequest::target_feed, 2, "target_feed", false);
    meta->prop(&zbase::CrawlRequest::userdata, 3, "userdata", false);
    meta->prop(
        &zbase::CrawlRequest::follow_redirects,
        4,
        "follow_redirects",
        false);
  };
};

}
#endif
