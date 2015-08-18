/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/option.h>
#include <stx/UnixTime.h>
#include <stx/io/inputstream.h>
#include <stx/io/outputstream.h>
#include <stx/uri.h>

using namespace stx;

namespace zbase {

struct TrackedEvent {
  TrackedEvent(
      UnixTime _time,
      String _evid,
      String _evtype,
      String _data);

  UnixTime time;
  String evid;
  String evtype;
  String data;
};

struct TrackedSession {
  std::string customer_key;
  std::string uuid;
  Vector<TrackedEvent> events;

  void insertLogline(
      const UnixTime& time,
      const String& evtype,
      const String& evid,
      const URI::ParamList& logline);

  void encode(OutputStream* os) const;
  void decode(InputStream* os);

};

} // namespace zbase
