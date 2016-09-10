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

#ifdef EVQL_VERSION
static const std::string kVersionString = EVQL_VERSION;
#else
static const std::string kVersionString = "unknown";
#endif

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

enum {
  EVQL_OP_HELLO                      = 0x5e00,
  EVQL_OP_PING                       = 0x0001,
  EVQL_OP_HEARTBEAT                  = 0x0002,
  EVQL_OP_ERROR                      = 0x0003,
  EVQL_OP_READY                      = 0x0004,
  EVQL_OP_BYE                        = 0x0005,
  EVQL_OP_QUERY                      = 0x0006,
  EVQL_OP_QUERY_RESULT               = 0x0007,
  EVQL_OP_QUERY_CONTINUE             = 0x0008,
  EVQL_OP_QUERY_DISCARD              = 0x0009,
  EVQL_OP_QUERY_PROGRESS             = 0x0010,
  EVQL_OP_QUERY_NEXT                 = 0x0011,
  EVQL_OP_QUERY_PARTIALAGGR          = 0x0012,
  EVQL_OP_QUERY_PARTIALAGGR_RESULT   = 0x0013
};

enum {
  EVQL_ENDOFREQUEST                   = 0x1
};

enum {
  EVQL_HELLO_INTERNAL                 = 0x1,
  EVQL_HELLO_SWITCHDB                 = 0x2,
  EVQL_HELLO_INTERACTIVEAUTH          = 0x4
};

enum {
  EVQL_QUERY_SWITCHDB     = 0x1,
  EVQL_QUERY_MULTISTMT    = 0x2,
  EVQL_QUERY_PROGRESS     = 0x4,
  EVQL_QUERY_NOSTATS      = 0x8
};

enum {
  EVQL_QUERY_RESULT_COMPLETE     = 0x1,
  EVQL_QUERY_RESULT_HASSTATS     = 0x2,
  EVQL_QUERY_RESULT_HASCOLNAMES  = 0x4,
  EVQL_QUERY_RESULT_PENDINGSTMT  = 0x8
};

/**
 * The EventQL client handle
 */
struct evql_client_s;
typedef struct evql_client_s evql_client_t;

/**
 * Create a new eventql client
 */
evql_client_t* evql_client_init();

/**
 * Set an auth parameter
 */
int evql_client_setauth(
    evql_client_t* client,
    const char* key,
    size_t key_len,
    const char* val,
    size_t val_len,
    long flags);

/**
 * Set an option
 */
int evql_client_setopt(
    evql_client_t* client,
    int opt,
    const char* val,
    size_t val_len,
    long flags);

/**
 * Connect to an eventql server
 */
int evql_client_connect(
    evql_client_t* client,
    const char* host,
    unsigned int port,
    const char* database,
    long flags);

/**
 * Connect to an eventql server
 */
int evql_client_connectfd(
    evql_client_t* client,
    int fd,
    long flags);

/**
 * Execute a query
 */
int evql_query(
    evql_client_t* client,
    const char* query_string,
    const char* database,
    long flags);

/**
 * Fetch the next row of the result table
 *
 * @return This method returns negative one (-1) on error, zero (0) if there
 * are no more rows to read (EOF) and positive one (1) if the next row was read.
 *
 * If the method returns with negative one or zero, the contents of the fields
 * and field_lenghts parameters are unchanged.
 */
int evql_fetch_row(
    evql_client_t* client,
    const char*** fields,
    size_t** field_lengths);

/**
 * Fetch a column name from the result
 */
int evql_column_name(
    evql_client_t* client,
    size_t column_index,
    const char** name,
    size_t* name_len);

/**
 * Get the number of fields in the result
 */
int evql_num_columns(evql_client_t* client, size_t* ncols);

/**
 * Discard the remainder of the result
 */
int evql_discard_result(evql_client_t* client);

/**
 * Continue with the next result
 *
 * @return This method returns negative one (-1) on error, zero (0) if there
 * are no more results to read (EOF) and positive one (1) if the next result
 * is available for reading.
 */
int evql_next_result(evql_client_t* client);

/**
 * Free the current result
 */
void evql_client_releasebuffers(evql_client_t* client);

/**
 * Return the latest error message
 */
const char* evql_client_geterror(evql_client_t* client);

/**
 * Close the connection gracefully
 */
int evql_client_close(evql_client_t* client);

/**
 * Destroy a eventql client
 */
void evql_client_destroy(evql_client_t* client);


/**
 * The EventQL config handle
 */
struct evql_conf_s;
typedef struct evql_conf_s evql_conf_t;

/**
 * Create a new EventQL server config
 */
evql_conf_t* evql_conf_init();

/**
 * Set a server config property
 */
void evql_conf_set(
    evql_conf_t* conf,
    const char* key,
    const char* value);

/**
 * Load config properties from a file
 *
 * @param fpath the file path or NULL for the default paths
 * @return 0 on success and 1 on error
 */
int evql_conf_load(
    evql_conf_t* conf,
    const char* fpath);

/**
 * Free an eventql server config
 */
void evql_conf_free(evql_conf_t* conf);


/**
 * The EventQL server handle
 */
struct evql_server_s;
typedef struct evql_server_s evql_server_t;

/**
 * Initiate a new EventQL server
 */
evql_server_t* evql_server_init(evql_conf_t* conf);

/**
 * Start the server (this method returns immediately)
 *
 * @return 0 on success and 1 on error
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
 *
 * @return 0 on success and 1 on error
 */
int evql_server_listen(evql_server_t* server, int kill_fd);

/**
 * Start a new EventQL server connection/thread. You must pass a valid file
 * descriptor. The server will internally start a new query thread and start
 * reading queries from the fd;
 *
 * @return 0 on success and 1 on error
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

/**
 * Get a server config property
 */
const char* evql_server_getconf(
    evql_server_t* server,
    const char* key);

/**
 * Get a boolean server config property
 */
int evql_server_getconfbool(
    evql_server_t* server,
    const char* key);

#ifdef __cplusplus
}
#endif

