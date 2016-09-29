/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include "eventql/eventql.h"
#include "eventql/util/stdtypes.h"
#include "eventql/util/return_code.h"

namespace eventql {
struct DatabaseContext;

// A session is _not_ thread safe
class Session {
public:

  Session(const DatabaseContext* database_context = nullptr);

  String getUserID() const;
  void setUserID(const String& user_id);

  String getEffectiveNamespace() const;
  void setEffectiveNamespace(const String& ns);

  String getDisplayNamespace() const;
  void setDisplayNamespace(const String& ns);

  const DatabaseContext* getDatabaseContext();

  void setHeartbeatCallback(std::function<ReturnCode ()> cb);
  ReturnCode triggerHeartbeat();

  void setIdleTimeout(uint64_t timeout_us);
  uint64_t getIdleTimeout() const;

  uint64_t getHeartbeatInterval() const;

  void setIsInternal(bool is_internal);
  uint64_t isInternal() const;

protected:
  mutable std::mutex mutex_;
  const DatabaseContext* database_context_;
  String user_id_;
  String effective_namespace_;
  String display_namespace_;
  std::function<ReturnCode ()> heartbeat_cb_;
  uint64_t heartbeat_last_;
  uint64_t heartbeat_interval_;
  uint64_t idle_timeout_;
  bool is_internal_;
};

} // namespace eventql
