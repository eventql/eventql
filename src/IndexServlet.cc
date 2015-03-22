/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "IndexServlet.h"

using namespace fnord;

namespace cm {

IndexServlet::IndexServlet(
    RefPtr<cm::IndexReader> index,
    RefPtr<fts::Analyzer> analyzer) :
    index_(index),
    analyzer_(analyzer) {}

void IndexServlet::handleHTTPRequest(
    fnord::http::HTTPRequest* req,
    fnord::http::HTTPResponse* res) {
}

}
