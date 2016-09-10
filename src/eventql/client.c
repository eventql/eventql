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
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <limits.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <assert.h>

#if CHAR_BIT != 8
#error "unsupported char size"
#endif

static const size_t EVQL_FRAME_MAX_SIZE = 1024 * 1024 * 256;
static const size_t EVQL_FRAME_HEADER_SIZE = 8;
static const size_t EVQL_CLIENT_DEFAULT_IO_TIMEOUT_US = 1000 * 1000  * 1; // 1 second
static const size_t EVQL_CLIENT_DEFAULT_IDLE_TIMEOUT_US = 1000 * 1000  * 5; // 5 seconds
static const size_t EVQL_CLIENT_DEFAULT_BATCH_SIZE = 1024;
#define EVQL_CLIENT_INLINE_RBUF_SIZE 64
#define EVQL_CLIENT_INLINE_CBUF_SIZE 1024

/**
 * Internal struct declarations
 */

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
  char* authdata;
  size_t authdata_len;
  size_t authdata_capacity;
  uint64_t io_timeout_us;
  uint64_t idle_timeout_us;
  size_t batch_size;
  evql_framebuf_t recv_buf;
  int qbuf_valid;
  int qbuf_islast;
  int qbuf_pendingstmt;
  size_t qbuf_nrows;
  size_t qbuf_ncols;
  const char** rbuf_ptrs;
  size_t* rbuf_lens;
  size_t rbuf_size;
  char rbuf_inline[
      (sizeof(const char*) + sizeof(size_t)) * EVQL_CLIENT_INLINE_RBUF_SIZE];
  char** cbuf;
  size_t cbuf_nbytes;
  size_t cbuf_nentries;
  char cbuf_inline[EVQL_CLIENT_INLINE_CBUF_SIZE];
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

static int evql_framebuf_readlenencint(
    evql_framebuf_t* frame,
    uint64_t* val);

static int evql_framebuf_readlenencstr(
    evql_framebuf_t* frame,
    const char** val,
    size_t* len);

static void evql_client_rbuf_alloc(evql_client_t* client, size_t rbuf_size);
static void evql_client_rbuf_free(evql_client_t* client);
static void evql_client_seterror(evql_client_t* client, const char* error);
static void evql_client_qbuf_free(evql_client_t* client);
static void evql_client_cbuf_free(evql_client_t* client);
static void evql_client_cbuf_alloc(evql_client_t* client, size_t cbuf_size);
static void evql_client_cbuf_set(
    evql_client_t* client,
    size_t n,
    const char** ptrs,
    size_t* lens);

static int evql_client_handshake(evql_client_t* client, const char* database);
static void evql_client_close_hard(evql_client_t* client);

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
    frame->length = len;
    return;
  }

  frame->capacity = len; // FIXME
  frame->length = len;
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

static int evql_framebuf_read(
    evql_framebuf_t* frame,
    const char** data,
    size_t len) {
  if (frame->cursor + len > frame->length) {
    return -1;
  } else {
    *data = frame->data + frame->cursor;
    frame->cursor += len;
    return 0;
  }
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

static int evql_framebuf_readlenencint(
    evql_framebuf_t* frame,
    uint64_t* val) {
  *val = 0;
  for (int i = 0; i < 10; ++i) {
    unsigned char* b;
    if (evql_framebuf_read(frame, (const char**) &b, 1) == -1) {
      return -1;
    }

    *val |= ((*b) & 0x7fULL) << (7 * i);

    if (!((*b) & 0x80U)) {
      break;
    }
  }

  return 0;
}

static int evql_framebuf_readlenencstr(
    evql_framebuf_t* frame,
    const char** val,
    size_t* len) {
  uint64_t strlen;
  if (evql_framebuf_readlenencint(frame, &strlen) == -1) {
    return -1;
  }

  *len = strlen;
  return evql_framebuf_read(frame, val, strlen);
}

static int evql_client_write(
    evql_client_t* client,
    const char* data,
    size_t len,
    uint64_t timeout_us) {
  size_t pos = 0;
  while (pos < len) {
    int write_rc = write(client->fd, data + pos, len - pos);
    switch (write_rc) {
      case 0:
        evql_client_seterror(client, "connection unexpectedly closed");
        evql_client_close_hard(client);
        return -1;
      case -1:
        if (errno == EAGAIN || errno == EINTR) {
          break;
        } else {
          evql_client_seterror(client, strerror(errno));
          evql_client_close_hard(client);
          return -1;
        }
      default:
        pos += write_rc;
        break;
    }

    if (pos == len) {
      break;
    }

    struct pollfd p;
    p.fd = client->fd;
    p.events = POLLOUT;
    int poll_rc = poll(&p, 1, timeout_us / 1000);
    switch (poll_rc) {
      case 0:
        evql_client_seterror(client, "operation timed out");
        evql_client_close_hard(client);
        return -1;
      case -1:
        if (errno == EAGAIN || errno == EINTR) {
          break;
        } else {
          evql_client_seterror(client, strerror(errno));
          evql_client_close_hard(client);
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
        evql_client_close_hard(client);
        return -1;
      case -1:
        if (errno == EAGAIN || errno == EINTR) {
          break;
        } else {
          evql_client_seterror(client, strerror(errno));
          evql_client_close_hard(client);
          return -1;
        }
      default:
        pos += read_rc;
        break;
    }

    if (pos == len) {
      break;
    }

    struct pollfd p;
    p.fd = client->fd;
    p.events = POLLIN;
    int poll_rc = poll(&p, 1, timeout_us / 1000);
    switch (poll_rc) {
      case 0:
        evql_client_seterror(client, "operation timed out");
        evql_client_close_hard(client);
        return -1;
      case -1:
        if (errno == EAGAIN || errno == EINTR) {
          break;
        } else {
          evql_client_seterror(client, strerror(errno));
          evql_client_close_hard(client);
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
    uint16_t flags) {
  uint16_t opcode_n = htons(opcode);
  uint16_t flags_n = htons(flags);
  uint32_t payload_len_n = htonl(frame->length);

  memcpy(frame->header + 0, (const char*) &opcode_n, 2);
  memcpy(frame->header + 2, (const char*) &flags_n, 2);
  memcpy(frame->header + 4, (const char*) &payload_len_n, 4);

  int ret = evql_client_write(
      client,
      frame->header,
      frame->length + 8,
      client->io_timeout_us);

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
      timeout_us);

  if (rc == -1) {
    return -1;
  }

  *opcode = ntohs(*((uint16_t*) &header[0]));
  uint16_t flags = ntohs(*((uint16_t*) &header[2]));
  uint32_t payload_len = ntohl(*((uint32_t*) &header[4]));
  if (recvflags) {
    *recvflags = flags;
  }

  if (payload_len > EVQL_FRAME_MAX_SIZE) {
    evql_client_seterror(client, "received invalid frame header");
    evql_client_close_hard(client);
    return -1;
  }

  evql_client_qbuf_free(client);
  evql_framebuf_clear(&client->recv_buf);
  evql_framebuf_resize(&client->recv_buf, payload_len);

  rc = evql_client_read(
      client,
      client->recv_buf.data,
      payload_len,
      client->io_timeout_us);

  if (rc == -1) {
    return -1;
  }

  *frame = &client->recv_buf;
  return 0;
}

static int evql_client_handshake(evql_client_t* client, const char* database) {
  /* send hello frame */
  {
    uint16_t hello_flags = 0;
    if (database) {
      hello_flags |= EVQL_HELLO_SWITCHDB;
    }

    evql_framebuf_t hello_frame;
    evql_framebuf_init(&hello_frame);
    evql_framebuf_writelenencint(&hello_frame, 1); // protocol version
    evql_framebuf_writelenencstr(&hello_frame, "eventql", 7); // eventql version
    evql_framebuf_writelenencint(&hello_frame, hello_flags); // flags
    evql_framebuf_writelenencint(&hello_frame, client->idle_timeout_us);
    evql_framebuf_writelenencint(&hello_frame, client->authdata_len);
    if (client->authdata) {
      evql_framebuf_write(&hello_frame, client->authdata, client->authdata_len);
    }
    if (database) {
      evql_framebuf_writelenencstr(&hello_frame, database, strlen(database));
    }

    int rc = evql_client_sendframe(
        client,
        EVQL_OP_HELLO,
        &hello_frame,
        0);

    evql_framebuf_destroy(&hello_frame);

    if (rc < 0) {
      evql_client_close_hard(client);
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
        client->idle_timeout_us,
        NULL);

    if (rc < 0) {
      evql_client_close_hard(client);
      return -1;
    }

    switch (response_opcode) {
      case EVQL_OP_READY:
        client->connected = 1;
        break;
      case EVQL_OP_ERROR: {
        const char* err;
        size_t err_len;
        int err_rc = evql_framebuf_readlenencstr(
            &client->recv_buf,
            &err,
            &err_len);

        if (err_rc == 0) {
          evql_client_seterror(client, err);
        } else {
          evql_client_seterror(client, "<unspecified error>");
        }

        evql_client_close_hard(client);
        return -1;
      }
      default:
        evql_client_seterror(client, "received unexpected opcode");
        evql_client_close_hard(client);
        return -1;
    }
  }

  return 0;
}

static int evql_client_query_readresultframe(evql_client_t* client) {
  int done = 0;
  uint16_t response_opcode;
  evql_framebuf_t* r_buf;
  while (!done) {
    int rc = evql_client_recvframe(
        client,
        &response_opcode,
        &r_buf,
        client->idle_timeout_us,
        NULL);

    if (rc < 0) {
      evql_client_close_hard(client);
      return -1;
    }

    switch (response_opcode) {
      case EVQL_OP_QUERY_RESULT:
        done = 1;
        break;

      case EVQL_OP_HEARTBEAT:
        break;

      case EVQL_OP_QUERY_PROGRESS:
        // FIXME handle query progress
        break;

      case EVQL_OP_ERROR: {
        const char* err;
        size_t err_len;
        if (evql_framebuf_readlenencstr(r_buf, &err, &err_len) == 0) {
          evql_client_seterror(client, err);
        } else {
          evql_client_seterror(client, "<unspecified error>");
        }
        return -1;
      }

      default:
        evql_client_seterror(client, "received unexpected opcode");
        evql_client_close_hard(client);
        return -1;
    }
  }

  assert(response_opcode == EVQL_OP_QUERY_RESULT);

  uint64_t r_flags;
  if (evql_framebuf_readlenencint(r_buf, &r_flags) == -1) {
    evql_client_seterror(client, "error while reading query result header");
    evql_client_close_hard(client);
    return -1;
  }

  uint64_t r_ncols;
  if (evql_framebuf_readlenencint(r_buf, &r_ncols) == -1) {
    evql_client_seterror(client, "error while reading query result header");
    evql_client_close_hard(client);
    return -1;
  }

  uint64_t r_nrows;
  if (evql_framebuf_readlenencint(r_buf, &r_nrows) == -1) {
    evql_client_seterror(client, "error while reading query result header");
    evql_client_close_hard(client);
    return -1;
  }

  if (r_flags & EVQL_QUERY_RESULT_HASSTATS) {
    uint64_t tmp;
    if (evql_framebuf_readlenencint(r_buf, &tmp) == -1) {
      evql_client_seterror(client, "error while reading query result header");
      evql_client_close_hard(client);
      return -1;
    }

    if (evql_framebuf_readlenencint(r_buf, &tmp) == -1) {
      evql_client_seterror(client, "error while reading query result header");
      evql_client_close_hard(client);
      return -1;
    }

    if (evql_framebuf_readlenencint(r_buf, &tmp) == -1) {
      evql_client_seterror(client, "error while reading query result header");
      evql_client_close_hard(client);
      return -1;
    }

    if (evql_framebuf_readlenencint(r_buf, &tmp) == -1) {
      evql_client_seterror(client, "error while reading query result header");
      evql_client_close_hard(client);
      return -1;
    }
  }

  if (r_flags & EVQL_QUERY_RESULT_HASCOLNAMES) {
    const char** cbuf_ptrs = alloca(sizeof(const char*) * r_ncols);
    size_t* cbuf_lens = alloca(sizeof(size_t) * r_ncols);

    for (size_t i = 0; i < r_ncols; ++i) {
      int rc = evql_framebuf_readlenencstr(r_buf, cbuf_ptrs + i, cbuf_lens + i);
      if (rc == -1) {
        evql_client_seterror(client, "error while reading query result header");
        evql_client_close_hard(client);
        return -1;
      }
    }

    evql_client_cbuf_set(client, r_ncols, cbuf_ptrs, cbuf_lens);
  }

  client->qbuf_valid = 1;
  client->qbuf_islast = (r_flags & EVQL_QUERY_RESULT_COMPLETE) > 0;
  client->qbuf_pendingstmt = (r_flags & EVQL_QUERY_RESULT_PENDINGSTMT) > 0;
  client->qbuf_nrows = r_nrows;
  client->qbuf_ncols = r_ncols;
  evql_client_rbuf_alloc(client, r_ncols);
  return 0;
}

static int evql_client_query_continue(evql_client_t* client) {
  if (!client->connected) {
    evql_client_seterror(client, "not connected");
    return -1;
  }

  /* send CONTINUE frame */
  {
    evql_framebuf_t cframe;
    evql_framebuf_init(&cframe);

    int rc = evql_client_sendframe(
        client,
        EVQL_OP_QUERY_CONTINUE,
        &cframe,
        0);

    evql_framebuf_destroy(&cframe);

    if (rc < 0) {
      evql_client_close_hard(client);
      return -1;
    }
  }

  /* receive response frames */
  return evql_client_query_readresultframe(client);
}

static void evql_client_close_hard(evql_client_t* client) {
  if (client->fd > 0) {
    close(client->fd);
  }

  client->fd = -1;
  client->connected = 0;
}

static void evql_client_rbuf_alloc(evql_client_t* client, size_t rbuf_size) {
  if (rbuf_size <= client->rbuf_size) {
    return;
  }

  if (client->rbuf_ptrs &&
      (void*) client->rbuf_ptrs != (void*) client->rbuf_inline) {
    free(client->rbuf_ptrs);
  }

  void* rbuf_alloc = malloc(
      sizeof(const char*) * rbuf_size +
      sizeof(size_t) * rbuf_size);

  if (!rbuf_alloc) {
    abort();
  }

  memset(
      rbuf_alloc,
      0,
      sizeof(const char*) * rbuf_size + sizeof(size_t) * rbuf_size);

  client->rbuf_ptrs = rbuf_alloc;
  client->rbuf_size = rbuf_size;
  client->rbuf_lens = (size_t*) (client->rbuf_ptrs + rbuf_size);
}

static void evql_client_rbuf_free(evql_client_t* client) {
  if (client->rbuf_ptrs &&
      (void*) client->rbuf_ptrs != (void*) client->rbuf_inline) {
    free(client->rbuf_ptrs);
  }

  client->rbuf_ptrs = (const char**) client->rbuf_inline;
  client->rbuf_lens = (size_t*) client->rbuf_ptrs + EVQL_CLIENT_INLINE_RBUF_SIZE;
  client->rbuf_size = EVQL_CLIENT_INLINE_RBUF_SIZE;
}

static void evql_client_cbuf_alloc(evql_client_t* client, size_t cbuf_len) {
  if (client->cbuf_nbytes >= cbuf_len) {
    return;
  }

  if (client->cbuf) {
    free(client->cbuf);
  }

  client->cbuf = malloc(cbuf_len); // FIXME
  client->cbuf_nbytes = cbuf_len;
  if (!client->cbuf) {
    abort();
  }
}

static void evql_client_cbuf_set(
    evql_client_t* client,
    size_t n,
    const char** ptrs,
    size_t* lens) {
  size_t cbuf_len = sizeof(const char*) * (n + 1);
  for (size_t i = 0; i < n; ++i) {
    cbuf_len += lens[i] + 1;
  }

  evql_client_cbuf_alloc(client, cbuf_len);
  client->cbuf_nentries = n;
  char* data = (char*) &client->cbuf[n + 1];
  client->cbuf[0] = data;
  for (size_t i = 0; i < n; ++i) {
    memcpy(data, ptrs[i], lens[i]);
    data[lens[i]] = 0;
    data += lens[i] + 1;
    client->cbuf[i + 1] = data;
  }
}

static void evql_client_cbuf_free(evql_client_t* client) {
  if (client->cbuf && (void*) client->cbuf != (void*) client->cbuf_inline) {
    free(client->cbuf);
  }

  client->cbuf = (char**) client->cbuf_inline;
  client->cbuf_nentries = 0;
  client->cbuf_nbytes = EVQL_CLIENT_INLINE_CBUF_SIZE;
}

static const char* EVQL_CLIENT_UNSPECIFIED_ERROR = "<unspecified error>";
const char* evql_client_geterror(evql_client_t* client) {
  if (client->error) {
    return client->error;
  } else {
    return EVQL_CLIENT_UNSPECIFIED_ERROR;
  }
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
  client->authdata = NULL;
  client->authdata_len = 0;
  client->authdata_capacity = 0;
  client->io_timeout_us = EVQL_CLIENT_DEFAULT_IO_TIMEOUT_US;
  client->idle_timeout_us = EVQL_CLIENT_DEFAULT_IDLE_TIMEOUT_US;
  client->batch_size = EVQL_CLIENT_DEFAULT_BATCH_SIZE;
  memset(client->rbuf_inline, 0, sizeof(client->rbuf_inline));
  client->rbuf_ptrs = (const char**) client->rbuf_inline;
  client->rbuf_lens = (size_t*) client->rbuf_ptrs + EVQL_CLIENT_INLINE_RBUF_SIZE;
  client->rbuf_size = EVQL_CLIENT_INLINE_RBUF_SIZE;
  client->qbuf_valid = 0;
  client->qbuf_islast = 0;
  client->qbuf_pendingstmt = 0;
  client->qbuf_nrows = 0;
  client->qbuf_ncols = 0;
  client->cbuf = (char**) client->cbuf_inline;
  client->cbuf_nentries = 0;
  client->cbuf_nbytes = EVQL_CLIENT_INLINE_CBUF_SIZE;
  evql_framebuf_init(&client->recv_buf);
  return client;
}

int evql_client_setauth(
    evql_client_t* client,
    const char* key,
    size_t key_len,
    const char* val,
    size_t val_len,
    long flags) {
  size_t elen = key_len + val_len + 2;
  if (client->authdata_len + elen > client->authdata_capacity) {
    client->authdata_capacity = client->authdata_len + elen; // FIXME

    if (client->authdata) {
      client->authdata = realloc(client->authdata, client->authdata_capacity);
    } else {
      client->authdata = malloc(client->authdata_capacity);
    }

    if (!client->authdata) {
      printf("malloc() failed\n");
      abort();
    }
  }

  char* dst = client->authdata + client->authdata_len;
  client->authdata_len += elen;

  memcpy(dst, key, key_len);
  dst += key_len;
  *(dst++) = 0;
  memcpy(dst, val, val_len);
  dst += val_len;
  *dst = 0;
  return 0;
}

int evql_client_setopt(
    evql_client_t* client,
    int opt,
    const char* val,
    size_t val_len,
    long flags) {
  switch (opt) {
    //case EVQL_CLIENT_IOTIMEOUT:
    // return 0;
    default:
      break;
  }

  evql_client_seterror(client, "invalid option");
  return -1;
}

int evql_client_connect(
    evql_client_t* client,
    const char* host,
    unsigned int port,
    const char* database,
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

  size_t nodelay = 1;
  setsockopt(client->fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));

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

  return evql_client_handshake(client, database);
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

  return evql_client_handshake(client, NULL);
}

int evql_query(
    evql_client_t* client,
    const char* query_string,
    const char* database,
    long flags) {
  if (!client->connected) {
    evql_client_seterror(client, "not connected");
    return -1;
  }

  /* send query frame */
  {
    evql_framebuf_t qframe;
    evql_framebuf_init(&qframe);
    evql_framebuf_writelenencstr(&qframe, query_string, strlen(query_string));
    evql_framebuf_writelenencint(&qframe, flags); // flags
    evql_framebuf_writelenencint(&qframe, 10); // max rows
    if (database) {
      evql_framebuf_writelenencstr(&qframe, database, strlen(database));
    }

    int rc = evql_client_sendframe(
        client,
        EVQL_OP_QUERY,
        &qframe,
        0);

    evql_framebuf_destroy(&qframe);

    if (rc < 0) {
      evql_client_close_hard(client);
      return -1;
    }
  }

  /* receive response frames */
  return evql_client_query_readresultframe(client);
}

int evql_fetch_row(
    evql_client_t* client,
    const char*** fields,
    size_t** field_lengths) {
  if (!client->qbuf_valid) {
    evql_client_seterror(client, "no active result");
    return -1;
  }

  while (client->qbuf_nrows == 0) {
    if (client->qbuf_islast) {
      return 0; // eof
    }

    /* fetch next piece */
    if (evql_client_query_continue(client) == -1) {
      return -1;
    }
  }

  for (int i = 0; i < client->qbuf_ncols; ++i) {
    int rc = evql_framebuf_readlenencstr(
          &client->recv_buf,
          &client->rbuf_ptrs[i],
          &client->rbuf_lens[i]);

    if (rc == -1) {
      evql_client_seterror(client, "invalid encoding");
      evql_client_close_hard(client);
      return -1;
    }
  }

  *fields = client->rbuf_ptrs;
  *field_lengths = client->rbuf_lens;
  --client->qbuf_nrows;
  return 1;
}

int evql_next_result(evql_client_t* client) {
  if (!client->connected) {
    evql_client_seterror(client, "not connected");
    return -1;
  }

  if (!client->qbuf_valid) {
    evql_client_seterror(client, "not active query");
    return -1;
  }

  if (!client->qbuf_islast) {
    evql_client_seterror(client, "current query not finished yet");
    return -1;
  }

  if (!client->qbuf_pendingstmt) {
    return 0;
  }

  /* send QUERY_NEXT frame */
  {
    evql_framebuf_t cframe;
    evql_framebuf_init(&cframe);

    int rc = evql_client_sendframe(
        client,
        EVQL_OP_QUERY_NEXT,
        &cframe,
        0);

    evql_framebuf_destroy(&cframe);

    if (rc < 0) {
      evql_client_close_hard(client);
      return -1;
    }
  }

  /* receive response frames */
  if (evql_client_query_readresultframe(client) == 0) {
    return 1;
  } else {
    return -1;
  }
}

int evql_column_name(
    evql_client_t* client,
    size_t column_index,
    const char** name,
    size_t* name_len) {
  if (!client->qbuf_valid) {
    evql_client_seterror(client, "no active result");
    return -1;
  }

  if (column_index >= client->cbuf_nentries) {
    evql_client_seterror(client, "index out of range");
    return -1;
  }

  *name = client->cbuf[column_index];
  *name_len = (client->cbuf[column_index + 1] - client->cbuf[column_index]) - 1;
  return 0;
}

int evql_num_columns(evql_client_t* client, size_t* ncols) {
  if (!client->qbuf_valid) {
    evql_client_seterror(client, "no active result");
    return -1;
  }

  *ncols = client->qbuf_ncols;
  return 0;
}

void evql_client_qbuf_free(evql_client_t* client) {
  if (!client->qbuf_valid) {
    return;
  }

  client->qbuf_valid = 0;
  client->qbuf_islast = 0;
  client->qbuf_pendingstmt = 0;
  client->qbuf_nrows = 0;
  client->qbuf_ncols = 0;

  for (int i = 0; i < client->rbuf_size; ++i) {
    client->rbuf_ptrs[i] = NULL;
    client->rbuf_lens[i] = 0;
  }
}

void evql_client_releasebuffers(evql_client_t* client) {
  evql_client_qbuf_free(client);
  evql_client_rbuf_free(client);
  evql_client_cbuf_free(client);
  //evql_framebuf_release(&client->recv_buf);
}

int evql_client_close(evql_client_t* client) {
  evql_client_releasebuffers(client);

  int rc = 0;
  if (client->fd > 0) {
    evql_framebuf_t bye_frame;
    evql_framebuf_init(&bye_frame);

    rc = evql_client_sendframe(
        client,
        EVQL_OP_BYE,
        &bye_frame,
        0);

    evql_framebuf_destroy(&bye_frame);
    close(client->fd);
  }

  client->fd = -1;
  client->connected = 0;
  return rc;
}

void evql_client_destroy(evql_client_t* client) {
  evql_client_close_hard(client);

  if (client->error) {
    free(client->error);
  }

  if (client->authdata) {
    free(client->authdata);
  }

  evql_client_qbuf_free(client);
  evql_client_rbuf_free(client);
  evql_client_cbuf_free(client);

  evql_framebuf_destroy(&client->recv_buf);
  free(client);
}

