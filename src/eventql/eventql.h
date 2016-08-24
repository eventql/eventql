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
#include <stdlib.h>

#ifdef __cplusplus
#include <string>

namespace eventql {

static const std::string kVersionString = EVQL_VERSION;

#ifdef EVQL_BUILDID
static const std::string kBuildID = EVQL_BUILDID;
#else
static const std::string kBuildID = "unknown";
#endif

}

#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The EventQL server handle
 */
struct evql_server_s;
typedef struct evql_server_s evql_server_t;

/**
 * Initiate a new EventQL server
 */
evql_server_t* evql_server_init();

/**
 * Set a server config property
 */
void evql_server_cfgset(
    evql_server_t* server,
    const char* key,
    const char* value);

/**
 * Start the server (this method returns immediately)
 */
int evql_server_start(evql_server_t* server);

/**
 * Listen to inbound network connections. Calling this method is optional -
 * you can use an EventQL server once it was started without listening to
 * conncetions.
 *
 * The kill_fd parameter is optional. If the paramter is set to -1, the method
 * will _never_ return. If the kill_fd parameter is set to a valid file
 * descriptor, the method will return once the kill_fd become readable.
 */
int evql_server_listen(evql_server_t* server, int kill_fd);

/**
 * Start a new EventQL server connection/thread. You must pass a valid file
 * descriptor. The server will internally start a new query thread and start
 * reading queries from the fd;
 */
int evql_server_handle(evql_server_t* server, int fd, int flags);

/**
 * Stop the EventQL server and return most of the held resources (including the
 * server lock). This method must be called after evql_server_start and before
 * evql_server_free.
 *
 * It is legal to start and stop the same server instance more than once.
 */
void evql_server_shutdown(evql_server_t* server);

/**
 * Free an eventql server instance.
 */
void evql_server_free(evql_server_t* server);

/**
 * Return the latest error message
 */
const char* evql_server_geterror(evql_server_t* server);

#ifdef __cplusplus
}
#endif

