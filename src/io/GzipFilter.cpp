#include <xzero/io/GzipFilter.h>
#include <xzero/Buffer.h>
#include <xzero/sysconfig.h>
#include <stdexcept>
#include <system_error>
#include <zlib.h>

namespace xzero {

GzipFilter::GzipFilter(int level) {
  z_.total_in = 0;
  z_.total_out = 0;
  z_.zalloc = Z_NULL;
  z_.zfree = Z_NULL;
  z_.opaque = Z_NULL;

  int wbits = 15 + 16;

  int rv = deflateInit2(&z_,
                        level,        // compression level
                        Z_DEFLATED,   // method
                        wbits,        // window bits (15=gzip compression,
                                      // 16=simple header, -15=raw deflate)
                        level,        // memory level (1..9)
                        Z_FILTERED);  // strategy

  if (rv != Z_OK)
    throw std::runtime_error("deflateInit2 failed.");
}

GzipFilter::~GzipFilter() {
  deflateEnd(&z_);
}

std::string GzipFilter::z_code(int code) const {
  char msg[128];
  switch (code) {
    case Z_NEED_DICT:
      return "Z_NEED_DICT";
    case Z_ERRNO:
      snprintf(msg, sizeof(msg), "Z_ERRNO (%s)", strerror(errno));
      return msg;
    case Z_STREAM_ERROR:
      return "Z_STREAM_ERROR";
    case Z_DATA_ERROR:
      return "Z_DATA_ERROR";
    case Z_MEM_ERROR:
      return "Z_MEM_ERROR";
    case Z_BUF_ERROR:
      return "Z_BUF_ERROR";
    case Z_VERSION_ERROR:
      return "Z_VERSION_ERROR";
    case Z_OK:
      return "Z_OK";
    case Z_STREAM_END:
      return "Z_STREAM_END";
    default:
      snprintf(msg, sizeof(msg), "Z_<%d>", code);
      return msg;
  }
}

void GzipFilter::filter(const BufferRef& input, Buffer* output, bool last) {
  int mode;
  int expectedResult;

  if (last) {
    mode = Z_FINISH;
    expectedResult = Z_STREAM_END;
  } else {
    mode = Z_SYNC_FLUSH; // or Z_NO_FLUSH
    expectedResult = Z_OK;
  }

  z_.next_in = (Bytef*)input.cbegin();
  z_.avail_in = input.size();

  output->reserve(output->size() + input.size() * 1.1 + 12 + 18);
  z_.next_out = (Bytef*)output->end();
  z_.avail_out = output->capacity() - output->size();

  do {
    if (output->size() > output->capacity() / 2) {
      output->reserve(output->capacity() + Buffer::CHUNK_SIZE);
      z_.next_out = (Bytef*)output->end();
      z_.avail_out = output->capacity() - output->size();
    }

    const int rv = deflate(&z_, mode);

    if (rv != expectedResult) {
      switch (rv) {
        case Z_NEED_DICT:
          throw std::runtime_error("zlib dictionary needed.");
        case Z_ERRNO:
          throw std::system_error(errno, std::system_category());
        case Z_STREAM_ERROR:
          throw std::runtime_error("Invalid Zlib compression level.");
        case Z_DATA_ERROR:
          throw std::runtime_error("Invalid or incomplete deflate data.");
        case Z_MEM_ERROR:
          throw std::runtime_error("Zlib out of memory.");
        case Z_BUF_ERROR:
          throw std::runtime_error("Zlib buffer error.");
        case Z_VERSION_ERROR:
          throw std::runtime_error("Zlib version mismatch.");
        default:
          throw std::runtime_error("Unknown Zlib deflate() error.");
      }
    }
  } while (z_.avail_out == 0);

  assert(z_.avail_in == 0);

  output->resize(output->capacity() - z_.avail_out);
}

} // namespace xzero
