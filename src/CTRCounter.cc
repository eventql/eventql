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

CTRCounterData CTRCounterData::load(const Buffer& buf) {
  abort(); // DEPRECATED
  CTRCounterData counter;

  util::BinaryMessageReader reader(buf.data(), buf.size());
  counter.num_views = *reader.readUInt64();
  counter.num_clicks = *reader.readUInt64();

  return counter;
}

void CTRCounterData::write(const CTRCounterData& counter, Buffer* buf) {
  abort(); // DEPRECATED
  util::BinaryMessageWriter writer(sizeof(CTRCounterData));
  writer.appendUInt64(counter.num_views);
  writer.appendUInt64(counter.num_clicks);
  buf->append(writer.data(), writer.size());
}

CTRCounterData::CTRCounterData() :
    num_views(0),
    num_clicks(0),
    num_clicked(0) {}

void CTRCounterData::merge(const CTRCounterData& other) {
  abort(); // DEPRECATED
  num_views += other.num_views;
  num_clicks += other.num_clicks;
}

}
