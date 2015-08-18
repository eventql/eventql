/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "zbase/TopSearchTermsApp.h"

using namespace stx;

namespace cm {

TopSearchTermsApp::TopSearchTermsApp(
    tsdb::TSDBClient* tsdb) :
    dproc::DefaultApplication("cm.shopstats"),
    tsdb_(tsdb) {

}

} // namespace cm
