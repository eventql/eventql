#include <xzero/http/HttpOutputCompressor.h>
#include <xzero/http/HttpOutputFilter.h>
#include <xzero/http/HttpOutput.h>
#include <xzero/http/HttpRequest.h>
#include <xzero/http/HttpResponse.h>
#include <xzero/Tokenizer.h>
#include <xzero/Buffer.h>
#include <algorithm>
#include <system_error>
#include <stdexcept>
#include <cstdlib>

#include <xzero/sysconfig.h>

#if defined(HAVE_ZLIB_H)
#include <zlib.h>
#endif

#if defined(HAVE_BZLIB_H)
#include <bzlib.h>
#endif

namespace xzero {

// {{{ HttpOutputCompressor::ZlibFilter
#if defined(HAVE_ZLIB_H)
class HttpOutputCompressor::ZlibFilter : public HttpOutputFilter {
 public:
  explicit ZlibFilter(int flags, int level);
  ~ZlibFilter();

  void filter(const BufferRef& input, Buffer* output) override;

 private:
  z_stream z_;
};

HttpOutputCompressor::ZlibFilter::ZlibFilter(int flags, int level) {
  z_.zalloc = Z_NULL;
  z_.zfree = Z_NULL;
  z_.opaque = Z_NULL;

  int rv = deflateInit2(&z_,
                        level,        // compression level
                        Z_DEFLATED,   // method
                        flags,        // window bits (15=gzip compression,
                                      // 16=simple header, -15=raw deflate)
                        level,        // memory level (1..9)
                        Z_FILTERED);  // strategy

  if (rv != Z_OK)
    throw std::runtime_error("deflateInit2 failed.");
}

HttpOutputCompressor::ZlibFilter::~ZlibFilter() {
  deflateEnd(&z_);
}

void HttpOutputCompressor::ZlibFilter::filter(const BufferRef& input,
                                              Buffer* output) {
  z_.total_out = 0;
  z_.next_in = (Bytef*)input.cbegin();
  z_.avail_in = input.size();

  output->reserve(output->size() + input.size() * 1.1 + 12 + 18);
  z_.next_out = (Bytef*)output->end();
  z_.avail_out = output->capacity() - output->size();

  do {
    if (output->size() > output->capacity() / 2) {
      output->reserve(output->capacity() + Buffer::CHUNK_SIZE);
      z_.avail_out = output->capacity() - output->size();
    }

    const int rv = deflate(&z_, Z_SYNC_FLUSH); // or: Z_FINISH with Z_NO_FLUSH for non-last

    if (rv != Z_OK) { // or: STREAM_END for last
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

  output->resize(z_.total_out);
}
#endif
// }}}
// {{{ HttpOutputCompressor::DeflateFilter
#if defined(HAVE_ZLIB_H)
class HttpOutputCompressor::DeflateFilter : public ZlibFilter {
 public:
  explicit DeflateFilter(int level) : ZlibFilter(-15 + 16, level) {}
};
#endif
// }}}
// {{{ HttpOutputCompressor::GzipFilter
#if defined(HAVE_ZLIB_H)
class HttpOutputCompressor::GzipFilter : public ZlibFilter {
 public:
  explicit GzipFilter(int level) : ZlibFilter(15 + 16, level) {}
};
#endif
// }}}
// {{{ HttpOutputCompressor
HttpOutputCompressor::HttpOutputCompressor()
    : contentTypes_(),              // no types
      level_(9),                    // best compression
      minSize_(256),                // 256 byte
      maxSize_(128 * 1024 * 1024) { // 128 MB
  addMimeType("text/plain");
  addMimeType("text/html");
  addMimeType("text/css");
  addMimeType("application/xml");
  addMimeType("application/xhtml+xml");
}

HttpOutputCompressor::~HttpOutputCompressor() {
}

void HttpOutputCompressor::setMinSize(size_t value) {
  minSize_ = value;
}

void HttpOutputCompressor::setMaxSize(size_t value) {
  maxSize_ = value;
}

void HttpOutputCompressor::addMimeType(const std::string& value) {
  contentTypes_[value] = 0;
}

bool HttpOutputCompressor::containsMimeType(const std::string& value) const {
  return contentTypes_.find(value) != contentTypes_.end();
}

template<typename Encoder>
bool tryEncode(const std::string& encoding,
               int level,
               const std::vector<BufferRef>& accepts,
               HttpRequest* request,
               HttpResponse* response) {
  if (std::find(accepts.begin(), accepts.end(), encoding) == accepts.end())
    return false;

  // response might change according to Accept-Encoding
  response->appendHeader("Vary", "Accept-Encoding", ",");

  // removing content-length implicitely enables chunked encoding
  response->resetContentLength();

  response->addHeader("Content-Encoding", encoding);
  response->output()->addFilter(std::make_shared<Encoder>(level));

  return true;
}

void HttpOutputCompressor::inject(HttpRequest* request,
                                  HttpResponse* response) {
  // TODO: ensure postProcess() gets invoked right before response commit
}

void HttpOutputCompressor::postProcess(HttpRequest* request,
                                       HttpResponse* response) {
  if (response->headers().contains("Content-Encoding"))
    return;  // do not double-encode content

  bool chunked = !response->hasContentLength();
  long long size = chunked ? 0 : response->contentLength();

  if (!chunked && (size < minSize_ || size > maxSize_))
    return;

  if (!containsMimeType(response->headers().get("Content-Type")))
    return;

  const std::string& acceptEncoding = request->headers().get("Accept-Encoding");
  BufferRef r(acceptEncoding);
  if (!r.empty()) {
    const auto items = Tokenizer<BufferRef, BufferRef>::tokenize(r, ", ");

    // if (tryEncode<Bzip2Filter>("bzip2", level_, items, request, response))
    //   return;

    if (tryEncode<GzipFilter>("gzip", level_, items, request, response))
      return;

    if (tryEncode<DeflateFilter>("deflate", level_, items, request, response))
      return;
  }
}
// }}}

} // namespace xzero
