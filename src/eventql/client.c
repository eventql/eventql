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

#if CHAR_BIT != 8
#error "unsupported char size"
#endif

static const size_t EVQL_DEFAULT_TIMEOUT_US = 1000 * 1000  * 1; // 1 second
static const size_t EVQL_FRAME_MAX_SIZE = 1024 * 1024 * 256;
static const size_t EVQL_FRAME_HEADER_SIZE = 8;

/**
 * Internal struct declarations
 */
enum {
  EVQL_OP_HELLO           = 0x5e00,
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
  char* header;
  char* data;
  size_t length;
  size_t capacity;
  size_t cursor;
  char header_inline[8];
} evql_framebuf_t;

struct evql_client_s {
  int fd;
  char* error;
  int connected;
  evql_framebuf_t recv_buf;
  uint64_t timeout_us;
};

/**
 * Internal API declarations
 */
static void evql_framebuf_init(evql_framebuf_t* frame);
static void evql_framebuf_destroy(evql_framebuf_t* frame);
static void evql_framebuf_clear(evql_framebuf_t* frame);
static void evql_framebuf_resize(evql_framebuf_t* frame, size_t len);

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
static void evql_client_close(evql_client_t* client);

static int evql_client_write(
    evql_client_t* client,
    const char* data,
    size_t len,
    uint64_t timeout_us);

static int evql_client_read(
    evql_client_t* client,
    char* data,
    size_t len,
    uint64_t timeout_us);

static int evql_client_sendframe(
    evql_client_t* client,
    uint16_t opcode,
    evql_framebuf_t* frame,
    uint64_t timeout_us,
    uint16_t flags);

static int evql_client_recvframe(
    evql_client_t* client,
    uint16_t* opcode,
    evql_framebuf_t** frame,
    uint64_t timeout_us,
    uint16_t* flags);

/**
 * Internal API implementation
 */

static void evql_framebuf_init(evql_framebuf_t* frame) {
  frame->header = frame->header_inline;
  frame->data = NULL;
  frame->length = 0;
  frame->capacity = 0;
  frame->cursor = 0;
}

static void evql_framebuf_destroy(evql_framebuf_t* frame) {
  if (frame->data) {
    free(frame->header);
  }
}

static void evql_framebuf_resize(evql_framebuf_t* frame, size_t len) {
  if (len <= frame->capacity) {
    return;
  }

  frame->length = len;
  frame->capacity = len;
  if (frame->data) {
    frame->header = realloc(
        frame->header,
        frame->capacity + EVQL_FRAME_HEADER_SIZE);
  } else {
    frame->header = malloc(frame->capacity + EVQL_FRAME_HEADER_SIZE);
  }

  if (!frame->header) {
    printf("malloc() failed\n");
    abort();
  }

  frame->data = frame->header + EVQL_FRAME_HEADER_SIZE;
}

static void evql_framebuf_clear(evql_framebuf_t* frame) {
  frame->cursor = 0;
  frame->length = 0;
}

static void evql_framebuf_write(
    evql_framebuf_t* frame,
    const char* data,
    size_t len) {
  size_t oldlen = frame->length;
  evql_framebuf_resize(frame, frame->length + len);
  memcpy(frame->data + oldlen, data, len);
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

static int evql_client_write(
    evql_client_t* client,
    const char* data,
    size_t len,
    uint64_t timeout_us) {
  size_t pos = 0;
  while (pos < len) {
    struct pollfd p;
    p.fd = client->fd;
    p.events = POLLOUT;

    int write_rc = write(client->fd, data + pos, len - pos);
    switch (write_rc) {
      case 0:
        evql_client_seterror(client, "connection unexpectedly closed");
        evql_client_close(client);
        return -1;
      case -1:
        if (errno == EAGAIN || errno == EINTR) {
          break;
        } else {
          evql_client_seterror(client, strerror(errno));
          evql_client_close(client);
          return -1;
        }
      default:
        pos += write_rc;
        break;
    }

    int poll_rc = poll(&p, 1, timeout_us / 1000);
    switch (poll_rc) {
      case 0:
        evql_client_seterror(client, "operation timed out");
        evql_client_close(client);
        return -1;
      case -1:
        if (errno == EAGAIN || errno == EINTR) {
          break;
        } else {
          evql_client_seterror(client, strerror(errno));
          evql_client_close(client);
          return -1;
        }
    }
  }

  return 0;
}

static int evql_client_read(
    evql_client_t* client,
    char* data,
    size_t len,
    uint64_t timeout_us) {
  size_t pos = 0;
  while (pos < len) {
    int read_rc = read(client->fd, data + pos, len - pos);
    switch (read_rc) {
      case 0:
        evql_client_seterror(client, "connection unexpectedly closed");
        evql_client_close(client);
        return -1;
      case -1:
        if (errno == EAGAIN || errno == EINTR) {
          break;
        } else {
          evql_client_seterror(client, strerror(errno));
          evql_client_close(client);
          return -1;
        }
      default:
        pos += read_rc;
        break;
    }

    struct pollfd p;
    p.fd = client->fd;
    p.events = POLLIN;

    int poll_rc = poll(&p, 1, timeout_us / 1000);
    switch (poll_rc) {
      case 0:
        evql_client_seterror(client, "operation timed out");
        evql_client_close(client);
        return -1;
      case -1:
        if (errno == EAGAIN || errno == EINTR) {
          break;
        } else {
          evql_client_seterror(client, strerror(errno));
          evql_client_close(client);
          return -1;
        }
    }
  }

  return 0;
}

static int evql_client_sendframe(
    evql_client_t* client,
    uint16_t opcode,
    evql_framebuf_t* frame,
    uint64_t timeout_us,
    uint16_t flags) {
  uint16_t opcode_n = htons(opcode);
  uint16_t flags_n = htons(flags);
  uint32_t payload_len_n = htonl(frame->length);

  memcpy(frame->header + 0, (const char*) &opcode_n, 2);
  memcpy(frame->header + 2, (const char*) &flags_n, 2);
  memcpy(frame->header + 4, (const char*) &payload_len_n, 4);

  int ret = evql_client_write(
      client, frame->header,
      frame->length + 8,
      client->timeout_us);

  if (ret == -1) {
    return -1;
  }

  return 0;
}

static int evql_client_recvframe(
    evql_client_t* client,
    uint16_t* opcode,
    evql_framebuf_t** frame,
    uint64_t timeout_us,
    uint16_t* recvflags) {
  char header[8];
  int rc = evql_client_read(
      client,
      header,
      sizeof(header),
      client->timeout_us);

  if (rc == -1) {
    return -1;
  }

  *opcode = htons(*((uint16_t*) &header[0]));
  uint16_t flags = htons(*((uint16_t*) &header[2]));
  uint32_t payload_len = htonl(*((uint32_t*) &header[4]));
  if (recvflags) {
    *recvflags = flags;
  }

  if (payload_len > EVQL_FRAME_MAX_SIZE) {
    evql_client_seterror(client, "received invalid frame header");
    evql_client_close(client);
    return -1;
  }


  evql_framebuf_clear(&client->recv_buf);
  evql_framebuf_resize(&client->recv_buf, payload_len);
  rc = evql_client_read(
      client,
      client->recv_buf.data,
      payload_len,
      client->timeout_us);

  if (rc == -1) {
    return -1;
  }

  *frame = &client->recv_buf;
  return 0;
}

static int evql_client_handshake(evql_client_t* client) {
  /* send hello frame */
  {
    evql_framebuf_t hello_frame;
    evql_framebuf_init(&hello_frame);
    evql_framebuf_writelenencint(&hello_frame, 1); // protocol version
    evql_framebuf_writelenencstr(&hello_frame, "eventql", 7); // eventql version
    evql_framebuf_writelenencint(&hello_frame, 0); // flags

    int rc = evql_client_sendframe(
        client,
        EVQL_OP_HELLO,
        &hello_frame,
        client->timeout_us,
        0);

    evql_framebuf_destroy(&hello_frame);

    if (rc < 0) {
      evql_client_close(client);
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
        &response_frame,
        client->timeout_us,
        NULL);

    if (rc < 0) {
      evql_client_close(client);
      return -1;
    }

    switch (response_opcode) {
      case EVQL_OP_READY:
        client->connected = 1;
        break;
      default:
        evql_client_seterror(client, "received unexpected opcode");
        evql_client_close(client);
        return -1;
    }
  }

  return 0;
}

static void evql_client_close(evql_client_t* client) {
  if (client->fd > 0) {
    close(client->fd);
  }

  client->fd = -1;
  client->connected = 0;
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
  client->timeout_us = EVQL_DEFAULT_TIMEOUT_US;
  return client;
}

int evql_client_connect(
    evql_client_t* client,
    const char* host,
    unsigned int port,
    long flags) {
  if (client->fd >= 0) {
    close(client->fd);
  }

  client->connected = 0;
  client->fd = -1;

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
    close(client->fd);
    client->fd = -1;
    return -1;
  }

  int old_flags = fcntl(client->fd, F_GETFL, 0);
  if (fcntl(client->fd, F_SETFL, old_flags | O_NONBLOCK) != 0) {
    evql_client_seterror(client, strerror(errno));
    close(client->fd);
    client->fd = -1;
    return -1;
  }

  return evql_client_handshake(client);
}

int evql_client_connectfd(
    evql_client_t* client,
    int fd,
    long flags) {
  if (client->fd >= 0) {
    close(client->fd);
  }

  client->connected = 0;
  client->fd = fd;

  int old_flags = fcntl(client->fd, F_GETFL, 0);
  if (fcntl(client->fd, F_SETFL, old_flags | O_NONBLOCK) != 0) {
    evql_client_seterror(client, strerror(errno));
    close(client->fd);
    client->fd = -1;
    return -1;
  }

  return evql_client_handshake(client);
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

