/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <stx/json/json.h>
#include <stx/random.h>
#include "zbase/JSONSink.h"

using namespace stx;

namespace zbase {

JSONSink::JSONSink() :
    buf_(new Buffer{}),
    json_(BufferOutputStream::fromBuffer(buf_.get())) {}

void JSONSink::open() {}

json::JSONOutputStream* JSONSink::json() {
  return &json_;
}

RefPtr<VFSFile> JSONSink::finalize() {
  return buf_.get();
}


} // namespace zbase

