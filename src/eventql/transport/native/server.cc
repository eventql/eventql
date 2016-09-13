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
#include "eventql/transport/native/server.h"
#include "eventql/transport/native/connection_tcp.h"
#include "eventql/transport/native/frames/ready.h"
#include "eventql/transport/native/frames/hello.h"
#include "eventql/util/logging.h"
#include "eventql/util/util/binarymessagereader.h"
#include "eventql/server/session.h"
#include "eventql/server/sql_service.h"
#include "eventql/server/rpc/partial_aggregate.h"
#include "eventql/sql/runtime/runtime.h"
#include "eventql/auth/client_auth.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <limits.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <netinet/tcp.h>

namespace eventql {
namespace native_transport {

Server::Server(Database* db) : db_(db) {}

void Server::startConnection(std::unique_ptr<NativeConnection> connection) {
  auto conn_ptr = connection.release();
  db_->startThread([this, conn_ptr] (Session* session) {
    std::unique_ptr<NativeConnection> conn(conn_ptr);

    auto rc = performHandshake(conn.get());
    if (!rc.isSuccess()) {
      logError("eventql", "Handshake error: $0", rc.getMessage());
      conn->close();
      return;
    }

    logDebug(
        "eventql",
        "Native connection established; remote=$0",
        conn->getRemoteHost());

    uint16_t opcode;
    uint16_t flags;
    std::string payload;
    bool cont = true;
    while (cont && rc.isSuccess()) {
      rc = conn->recvFrame(
          &opcode,
          &flags,
          &payload,
          session->getIdleTimeout());

      if (!rc.isSuccess()) {
        break;
      }

      switch (opcode) {
        case EVQL_OP_BYE:
          cont = false;
          break;
        default:
          rc = performOperation(conn.get(), opcode, payload);
          break;
      }
    }

    conn->close();
  });
}

ReturnCode Server::performHandshake(NativeConnection* conn) {
  auto config = db_->getConfig();
  auto session = db_->getSession();
  auto dbctx = session->getDatabaseContext();

  /* read HELLO frame */
  HelloFrame hello_frame;
  {
    uint16_t opcode;
    uint16_t flags;
    std::string payload;
    auto rc = conn->recvFrame(
        &opcode,
        &flags,
        &payload,
        session->getIdleTimeout());

    if (!rc.isSuccess()) {
      conn->close();
      return rc;
    }

    switch (opcode) {
      case EVQL_OP_HELLO:
        break;
      default:
        conn->sendErrorFrame("invalid opcode");
        return ReturnCode::error("ERUNTIME", "invalid opcode");
    }

    MemoryInputStream payload_is(payload.data(), payload.size());
    rc = hello_frame.readFrom(&payload_is);
    if (!rc.isSuccess()) {
      conn->sendErrorFrame("invalid HELLO frame");
      return rc;
    }
  }

  /* check that client idle timeout is valid */
  if (hello_frame.getIdleTimeout() < session->getHeartbeatInterval()) {
    conn->sendErrorFrame(
        StringUtil::format(
            "idle timeout too short, minimum is: $0us",
            session->getHeartbeatInterval()));

    return ReturnCode::error("ERUNTIME", "client timeout to short");
  }

  /* set timeouts */
  if (hello_frame.isInternal()) {
    session->setIdleTimeout(config->getInt("server.s2s_idle_timeout").get());
    conn->setIOTimeout(config->getInt("server.s2s_io_timeout").get());
  } else {
    session->setIdleTimeout(config->getInt("server.c2s_idle_timeout").get());
    conn->setIOTimeout(config->getInt("server.c2s_io_timeout").get());
  }

  /* auth */
  if (hello_frame.isInternal()) {
    auto auth_rc = InternalAuth::authenticateInternal(
        session,
        conn,
        hello_frame.getAuthData());

    if (!auth_rc.isSuccess()) {
      conn->sendErrorFrame(auth_rc.getMessage());
      return auth_rc;
    }
  } else {
    std::unordered_map<std::string, std::string> auth_data;
    for (const auto& p : hello_frame.getAuthData()) {
      auth_data.insert(p);
    }

    auto auth_rc = dbctx->client_auth->authenticateNonInteractive(
        session,
        auth_data);

    if (!auth_rc.isSuccess() &&
        hello_frame.getInteractiveAuth() &&
        dbctx->client_auth->authenticateInteractiveSupported()) {
      auth_rc = dbctx->client_auth->authenticateInteractive(session, conn);
    }

    if (!auth_rc.isSuccess()) {
      conn->sendErrorFrame(auth_rc.message());
      return ReturnCode::error("EAUTH", auth_rc.message());
    }
  }

  /* switch database */
  if (hello_frame.hasDatabase()) {
    auto rc = dbctx->client_auth->changeNamespace(
        session,
        hello_frame.getDatabase());

    if (!rc.isSuccess()) {
      return conn->sendErrorFrame(rc.message());
    }
  }

  /* send READY frame */
  {
    ReadyFrame ready_frame;
    ready_frame.setIdleTimeout(session->getIdleTimeout());

    std::string payload;
    auto payload_os = StringOutputStream::fromString(&payload);
    ready_frame.writeTo(payload_os.get());

    auto rc = conn->sendFrame(EVQL_OP_READY, 0, payload.data(), payload.size());
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  return ReturnCode::success();
}

ReturnCode Server::performOperation(
    NativeConnection* conn,
    uint16_t opcode,
    const std::string& payload) {
  logDebug("eventql", "Performing operation; opcode=$0", opcode);

  switch (opcode) {
    case EVQL_OP_QUERY:
      return performOperation_QUERY(db_, conn, payload);
    case EVQL_OP_QUERY_PARTIALAGGR:
      return performOperation_QUERY_PARTIALAGGR(
          db_,
          conn,
          payload.data(),
          payload.size());
    default:
      conn->sendErrorFrame("invalid opcode");
      conn->close();
      return ReturnCode::error("ERUNTIME", "invalid opcode");
  }

  return ReturnCode::success();
}

} // namespace native_transport
} // namespace eventql

