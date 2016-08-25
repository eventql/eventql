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
#include <eventql/eventql.h>
#include <string.h>

/**
 * Internal struct declarations
 */
struct evql_client_s {
  int fd;
  char* error;
  int connected;
};

enum {
  EVQL_OP_HELLO           = 0x005e,
  EVQL_OP_PING            = 0x0001,
  EVQL_OP_ERROR           = 0x0002,
  EVQL_OP_ACK             = 0x0003,
  EVQL_OP_KILL            = 0x0004,
  EVQL_OP_KILLED          = 0x0005,
  EVQL_OP_QUERY           = 0x0006,
  EVQL_OP_QUERY_RESULT    = 0x0007,
  EVQL_OP_QUERY_CONTINUE  = 0x0008,
  EVQL_OP_QUERY_DISCARD   = 0x0008,
  EVQL_OP_QUERY_PROGRESS  = 0x0009,
  EVQL_OP_READY           = 0x0010
};

typedef struct {
  char* payload;
  size_t payload_len;
  size_t payload_capacity;
} evql_framebuf_t;


/**
 * Internal API declarations
 */
static evql_framebuf_t* evql_framebuf_init();
static void evql_framebuf_free(evql_framebuf_t* frame);
static evql_framebuf_t* evql_framebuf_writelenencint(uint64_t val);
static evql_framebuf_t* evql_framebuf_writelenencstr(
    const char* val,
    size_t len);

static void evql_client_seterror(evql_client_t* client, const char* error);

static int evql_client_handshake(evql_client_t* client);

static int evql_client_sendframe(
    evql_client_t* client,
    uint16_t opcode,
    const char* payload,
    size_t payload_len);

static int evql_client_recvframe(
    evql_client_t* client,
    uint16_t* opcode,
    evql_framebuf_t** frame);

/**
 * Internal API implementation
 */

static evql_framebuf_t* evql_framebuf_init() {
  evql_framebuf_t* frame = malloc(sizeof(evql_framebuf_t));
  if (!frame) {
    return NULL;
  }

  frame->payload = NULL;
  frame->payload_len = 0;
  frame->payload_capacity = 0;
  return frame;
}

static void evql_framebuf_free(evql_framebuf_t* frame) {
  if (frame->payload) {
    free(frame->payload);
  }

  free(frame);
}

static int evql_client_sendframe(
    evql_client_t* client,
    uint16_t opcode,
    const char* payload,
    size_t payload_len) {
  evql_client_seterror(client, "sendframe not yet implemented");
  return -1;
}

static int evql_client_recvframe(
    evql_client_t* client,
    uint16_t* opcode,
    evql_framebuf_t** frame) {
  evql_client_seterror(client, "recvframe not yet implemented");
  return -1;
}

static int evql_client_handshake(evql_client_t* client) {
  /* send hello frame */
  {
    evql_framebuf_t* hello_frame = evql_framebuf_init();
    if (!hello_frame) {
      evql_client_seterror(client, "error while allocating hello frame");
      return -1;
    }

    evql_framebuf_writelenencint(1); // protocol version
    evql_framebuf_writelenencstr("eventql") // eventql version
    evql_framebuf_writelenencint(0); // flags

    int rc = evql_client_sendframe(
        client,
        EVQL_OP_HELLO,
        hello_frame->payload,
        hello_frame->payload_len);

    evql_framebuf_free(hello_frame);

    if (rc < 0) {
      return -1;
    }
  }

  /* receive response frame */
  {
    uint16_t response_opcode;
    evql_framebuf_t* response_frame;
    int rc = evql_client_recvframe(
        client,
        &response_opcode,
        &response_frame);

    if (rc < 0) {
      return -1;
    }

    switch (response_opcode) {
      case EVQL_OP_READY:
        client->connected = 1;
        break;
      default:
        evql_client_seterror(client, "received unexpected opcode");
        return -1;
    }
  }

  return 0;
}

static void evql_client_seterror(evql_client_t* client, const char* error) {
  if (client->error) {
    free(client->error);
  }

  size_t error_len = strlen(error);
  client->error = malloc(error_len + 1);
  if (!client) {
    return;
  }

  client->error[error_len] = 0;
  memcpy(client->error, error, error_len);
}

/**
 * Public API implementation
 */
evql_client_t* evql_client_init() {
  evql_client_t* client = malloc(sizeof(evql_client_t));
  if (!client) {
    return NULL;
  }

  client->fd = -1;
  client->error = NULL;
  client->connected = 0;
  return client;
}

void evql_client_destroy(evql_client_t* client) {
  if (client->error) {
    free(client->error);
  }

  free(client);
}

