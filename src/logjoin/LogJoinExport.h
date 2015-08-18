/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_LOGJOINEXPORT_H
#define _CM_LOGJOINEXPORT_H
#include "stx/stdtypes.h"
#include "stx/http/httpconnectionpool.h"
#include "brokerd/BrokerClient.h"
#include "zbase/JoinedSession.pb.h"

using namespace stx;

namespace zbase {

class LogJoinExport {
public:

  LogJoinExport(http::HTTPConnectionPool* http);

  void exportSession(const JoinedSession& session);

protected:

  void exportPreferenceSetFeed(const JoinedSession& session);
  void exportQueryFeed(const JoinedSession& session);

  feeds::BrokerClient broker_;
};

} // namespace zbase
#endif
