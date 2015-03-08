/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <fnord-base/util/binarymessagereader.h>
#include <fnord-base/util/binarymessagewriter.h>
#include "CTRCounter.h"

using namespace fnord;

namespace cm {

CTRCounter CTRCounter::load(const Buffer& buf) {
  CTRCounter counter;

  util::BinaryMessageReader reader(buf.data(), buf.size());
  counter.num_views = *reader.readUInt64();
  counter.num_clicks = *reader.readUInt64();

  return counter;
}

void CTRCounter::write(const CTRCounter& counter, Buffer* buf) {
  util::BinaryMessageWriter writer(sizeof(CTRCounter));
  writer.appendUInt64(counter.num_views);
  writer.appendUInt64(counter.num_clicks);
  buf->append(writer.data(), writer.size());
}

CTRCounter::CTRCounter() :
    num_views(0),
    num_clicks(0) {}

void CTRCounter::merge(const CTRCounter& other) {
  num_views += other.num_views;
  num_clicks += other.num_clicks;
}

}
