/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_ECOMMERCESEARCHQUERIESFEED_H
#define _CM_ECOMMERCESEARCHQUERIESFEED_H

#include "zbase/JoinedSession.pb.h"
#include "zbase/Report.h"
#include "zbase/TSDBTableScanSource.h"
#include "zbase/JSONSink.h"

using namespace stx;

namespace cm {

class ECommerceSearchQueriesFeed : public ReportRDD {
public:

  ECommerceSearchQueriesFeed(
      RefPtr<TSDBTableScanSource<JoinedSession>> input,
      RefPtr<JSONSink> output);

  void onInit();
  void onFinish();

  String contentType() const override;

protected:
  void onSession(const JoinedSession& session);

  size_t n_;
  RefPtr<TSDBTableScanSource<JoinedSession>> input_;
  RefPtr<JSONSink> output_;
};

} // namespace cm

#endif
