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
#include <unistd.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netdb.h>

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

static void evql_framebuf_write(
    evql_framebuf_t* frame,
    const char* data,
    size_t len);

static void evql_framebuf_writelenencint(
    evql_framebuf_t* frame,
    uint64_t val);

static void evql_framebuf_writelenencstr(
    evql_framebuf_t* frame,
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

static void evql_framebuf_write(
    evql_framebuf_t* frame,
    const char* data,
    size_t len) {
}

static void evql_framebuf_writelenencint(
    evql_framebuf_t* frame,
    uint64_t val) {
  unsigned char buf[10];
  size_t bytes = 0;
  do {
    buf[bytes] = val & 0x7fU;
    if (val >>= 7) buf[bytes] |= 0x80U;
    ++bytes;
  } while (val);

  evql_framebuf_write(frame, (const char*) buf, bytes);
}

static void evql_framebuf_writelenencstr(
    evql_framebuf_t* frame,
    const char* val,
    size_t len) {
  evql_framebuf_writelenencint(frame, len);
  evql_framebuf_write(frame, val, len);
}

static int evql_client_sendframe(
    evql_client_t* client,
    uint16_t opcode,
    const char* payload,
    size_t payload_len) {
  const char header[8];

  struct iovec iov[2];
  iov[0].iov_base = (void*) header;
  iov[0].iov_len = sizeof(header);
  iov[1].iov_base = (void*) payload;
  iov[1].iov_len = payload_len;

  int ret = writev(client->fd, iov, 2);
  if (ret < 0) {
    evql_client_seterror(client, "writev() failed");
    return -1;
  }

  return 0;
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

    evql_framebuf_writelenencint(hello_frame, 1); // protocol version
    evql_framebuf_writelenencstr(hello_frame, "eventql", 7); // eventql version
    evql_framebuf_writelenencint(hello_frame, 0); // flags

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

int evql_client_connect(
    evql_client_t* client,
    const char* host,
    unsigned int port,
    long flags) {
  if (client->fd >= 0) {
    close(client->fd);
    client->fd = -1;
  }

  if (client->connected) {
    client->connected = 0;
  }

  struct hostent* h = gethostbyname(host);
  if (h == NULL) {
    evql_client_seterror(client, "gethostbyname() failed");
    return -1;
  }

  client->fd = socket(AF_INET, SOCK_STREAM, 0);
  if (client->fd < 0) {
    evql_client_seterror(client, "socket() creation failed");
    return -1;
  }

  struct sockaddr_in saddr;
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(port);
  memcpy(&(saddr.sin_addr), h->h_addr, sizeof(saddr.sin_addr));
  memset(&(saddr.sin_zero), 0, 8);

  int rc = connect(
      client->fd,
      (const struct sockaddr *) &saddr,
      sizeof(saddr));

  if (rc < 0) {
    evql_client_seterror(client, "connect() failed");
  } else {
    rc = evql_client_handshake(client);
  }

  if (rc < 0) {
    close(client->fd);
    client->fd = -1;
  }

  return rc;
}

int evql_client_connectfd(
    evql_client_t* client,
    int fd,
    long flags) {
  if (client->fd >= 0) {
    close(client->fd);
    client->fd = -1;
  }

  if (client->connected) {
    client->connected = 0;
  }

  client->fd = fd;
  if (evql_client_handshake(client) == 0) {
    return 0;
  } else {
    close(client->fd);
    client->fd = -1;
    return -1;
  }
}

void evql_client_destroy(evql_client_t* client) {
  if (client->fd >= 0) {
    close(client->fd);
  }

  if (client->error) {
    free(client->error);
  }

  free(client);
}

const char* evql_client_geterror(evql_client_t* client) {
  return client->error;
}

