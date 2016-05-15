/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#pragma once
#include "eventql/util/stdtypes.h"
#include "eventql/util/autoref.h"
#include "eventql/util/duration.h"
#include "eventql/util/exception.h"
#include "eventql/util/Serializable.h"
#include "eventql/util/io/inputstream.h"

namespace util {
namespace rpc {
class RPCContext;

class RPCRequest : public RefCounted {
  friend class RPCContext;
public:

  RPCRequest(Function<void (RPCContext* ctx)> call_fn);

  void run();
  void cancel();

  void wait() const;
  bool waitFor(const Duration& timeout) const;

  void onReady(Function<void ()> on_ready);

  template <typename EventType>
  void onEvent(
      const String& event_name,
      const EventType& event);

protected:

  void returnSuccess();
  void returnError(const StandardException& e);
  void returnError(const ExceptionType type, const String& message);

  Function<void (RPCContext* ctx)> call_fn_;
  bool cancelled_;
  bool ready_;
  const char* error_;
  String error_message_;
  bool running_;
  HashMap<String, Function<void (const Serializable& result)>> on_event_;
  Function<void ()> on_ready_;
  Function<void ()> on_cancel_;
  mutable std::mutex mutex_;
  mutable std::condition_variable cv_;
};


} // namespace rpc
} // namespace util

