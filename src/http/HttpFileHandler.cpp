#include <xzero/http/HttpFileHandler.h>
#include <xzero/http/HttpRequest.h>
#include <xzero/http/HttpResponse.h>
#include <xzero/http/HttpOutput.h>
#include <xzero/http/HttpRangeDef.h>
#include <xzero/http/HeaderFieldList.h>
#include <xzero/io/FileRef.h>
#include <xzero/DateTime.h>
#include <xzero/Buffer.h>
#include <xzero/sysconfig.h>

#include <system_error>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#if 0
#define TRACE(level, msg...) do { \
  printf("HttpFile/%d: ", (level)); \
  printf(msg); \
  printf("\n"); \
} while (0)
#else
#define TRACE(level, msg...) do {} while (0)
#endif

namespace xzero {

// {{{ HttpFile
HttpFile::HttpFile(const std::string& path, const std::string& mimetype,
                   bool mtime, bool size, bool inode)
    : path_(path),
      errno_(0),
      mimetype_(mimetype),
      etagConsiderMTime_(mtime),
      etagConsiderSize_(size),
      etagConsiderINode_(inode),
      etag_(),
      lastModified_(),
      stat_() {
  TRACE(2, "(%s).ctor", path_.c_str());
  update();
}

HttpFile::~HttpFile() {
  TRACE(2, "(%s).dtor", path_.c_str());
}

std::string HttpFile::filename() const {
  std::size_t n = path_.rfind('/');
  return n != std::string::npos ? path_.substr(n + 1) : path_;
}

const std::string& HttpFile::etag() const {
  // compute ETag response header value on-demand
  if (etag_.empty()) {
    size_t count = 0;
    Buffer buf(256);
    buf.push_back('"');

    if (etagConsiderMTime_) {
      if (count++) buf.push_back('-');
      buf.push_back(stat_.st_mtime);
    }

    if (etagConsiderSize_) {
      if (count++) buf.push_back('-');
      buf.push_back(stat_.st_size);
    }

    if (etagConsiderINode_) {
      if (count++) buf.push_back('-');
      buf.push_back(stat_.st_ino);
    }

    buf.push_back('"');

    etag_ = buf.str();
  }

  return etag_;
}

const std::string& HttpFile::lastModified() const {
  // build Last-Modified response header value on-demand
  if (lastModified_.empty()) {
    if (struct tm* tm = std::gmtime(&stat_.st_mtime)) {
      char buf[256];

      if (std::strftime(buf, sizeof(buf), "%a, %d %b %Y %T GMT", tm) != 0) {
        lastModified_ = buf;
      }
    }
  }

  return lastModified_;
}

void HttpFile::update() {
#if 0
  int rv = fstat(fd_, &stat_);
#else
  int rv = stat(path_.c_str(), &stat_);
#endif

  if (rv < 0)
    errno_ = errno;
}

int HttpFile::tryCreateChannel() {
  int flags = O_RDONLY | O_NONBLOCK;

#if 0  // defined(O_NOATIME)
  flags |= O_NOATIME;
#endif

#if defined(O_LARGEFILE)
  flags |= O_LARGEFILE;
#endif

#if defined(O_CLOEXEC)
  flags |= O_CLOEXEC;
#endif

  return ::open(path_.c_str(), flags);
}
// }}}

// {{{ helper methods
/**
 * converts a range-spec into real offsets.
 *
 * \todo Mark this fn as \c constexpr as soon Ubuntu's LTS default compiler
 *supports it (14.04)
 */
inline /*constexpr*/ std::pair<std::size_t, std::size_t> makeOffsets(
    const std::pair<std::size_t, std::size_t>& p, std::size_t actualSize) {
  return p.first == HttpRangeDef::npos
             ? std::make_pair(actualSize - p.second,
                              actualSize - 1)  // last N bytes
             : p.second == HttpRangeDef::npos && p.second > actualSize
                   ? std::make_pair(p.first, actualSize - 1)  // from fixed N to
                                                              // the end of file
                   : std::make_pair(p.first, p.second);       // fixed range
}

/**
 * generates a boundary tag.
 *
 * \return a value usable as boundary tag.
 */
inline std::string generateBoundaryID() {
  static const char* map = "0123456789abcdef";
  char buf[16 + 1];

  for (std::size_t i = 0; i < sizeof(buf) - 1; ++i)
    buf[i] = map[random() % (sizeof(buf) - 1)];

  buf[sizeof(buf) - 1] = '\0';

  return std::string(buf);
}
// }}}

HttpFileHandler::HttpFileHandler(bool mtime, bool size, bool inode)
    : etagConsiderMTime_(mtime),
      etagConsiderSize_(size),
      etagConsiderINode_(inode) {
}

HttpFileHandler::~HttpFileHandler() {
}

void HttpFileHandler::configureETag(bool mtime, bool size, bool inode) {
  etagConsiderMTime_ = mtime;
  etagConsiderSize_ = size;
  etagConsiderINode_ = inode;
}

bool HttpFileHandler::handle(HttpRequest* request, HttpResponse* response,
                             const std::string& docroot) {
  std::string path = docroot + request->path(); //TODO:docroot-escape protection
  std::string mimetype; // TODO fill
  HttpFile transferFile(path, mimetype, etagConsiderMTime_, etagConsiderSize_,
                        etagConsiderINode_);

  if (handleClientCache(transferFile, request, response))
    return true;

  switch (transferFile.errorCode()) {
    case 0:
      break;
    case ENOENT:
      response->setStatus(HttpStatus::NotFound);
      response->completed();
      return true;
    case EACCES:
    case EPERM:
      response->setStatus(HttpStatus::Forbidden);
      response->completed();
      return true;
    default:
      throw std::system_error(transferFile.errorCode(), std::system_category());
  }

  int fd = -1;
  if (equals(request->method(), "GET")) {
    fd = transferFile.tryCreateChannel();
    if (fd < 0) {
      if (errno != EPERM && errno != EACCES)
        throw std::system_error(transferFile.errorCode(), std::system_category());

      response->setStatus(HttpStatus::Forbidden);
      response->completed();
      return true;
    }
  } else if (!equals(request->method(), "HEAD")) {
    response->setStatus(HttpStatus::MethodNotAllowed);
    response->completed();
    return true;
  }

  response->addHeader("Last-Modified", transferFile.lastModified());
  response->addHeader("ETag", transferFile.etag());

  if (handleRangeRequest(transferFile, fd, request, response))
    return true;

  response->setStatus(HttpStatus::Ok);
  response->addHeader("Accept-Ranges", "bytes");
  response->addHeader("Content-Type", transferFile.mimetype());
  response->addHeader("Content-Length", std::to_string(transferFile.size()));

  if (fd >= 0) {  // GET request
#if defined(HAVE_POSIX_FADVISE)
    posix_fadvise(fd, 0, transferFile.size(), POSIX_FADV_SEQUENTIAL);
#endif
    response->output()->write(FileRef(fd, 0, transferFile.size(), true));
  }

  response->completed();
  return true;
}

bool HttpFileHandler::handleClientCache(const HttpFile& transferFile,
                                        HttpRequest* request,
                                        HttpResponse* response) {
  // If-None-Match
  do {
    std::string value = request->headers().get("If-None-Match");
    if (value.empty()) continue;

    // XXX: on static files we probably don't need the token-list support
    if (value != transferFile.etag()) continue;

    response->setStatus(HttpStatus::NotModified);
    response->completed();
    return true;
  } while (0);

  // If-Modified-Since
  do {
    std::string value = request->headers().get("If-Modified-Since");
    if (value.empty()) continue;

    DateTime dt(value);
    if (!dt.valid()) continue;

    if (transferFile.mtime() > dt.unixtime()) continue;

    response->setStatus(HttpStatus::NotModified);
    response->completed();
    return true;
  } while (0);

  // If-Match
  do {
    BufferRef value = request->headers().get("If-Match");
    if (value.empty()) continue;

    if (value == "*") continue;

    // XXX: on static files we probably don't need the token-list support
    if (value == transferFile.etag()) continue;

    response->setStatus(HttpStatus::PreconditionFailed);
    response->completed();
    return true;
  } while (0);

  // If-Unmodified-Since
  do {
    BufferRef value = request->headers().get("If-Unmodified-Since");
    if (value.empty()) continue;

    DateTime dt(value);
    if (!dt.valid()) continue;

    if (transferFile.mtime() <= dt.unixtime()) continue;

    response->setStatus(HttpStatus::PreconditionFailed);
    response->completed();
    return true;
  } while (0);

  return false;
}

bool HttpFileHandler::handleRangeRequest(const HttpFile& transferFile, int fd,
                                         HttpRequest* request,
                                         HttpResponse* response) {
  const bool isHeadReq = fd < 0;
  BufferRef range_value(request->headers().get("Range"));
  HttpRangeDef range;

  // if no range request or range request was invalid (by syntax) we fall back
  // to a full response
  if (range_value.empty() || !range.parse(range_value))
    return false;

  std::string ifRangeCond = request->headers().get("If-Range");
  if (!ifRangeCond.empty()
        && !equals(ifRangeCond, transferFile.etag())
        && !equals(ifRangeCond, transferFile.lastModified()))
    return false;

  response->setStatus(HttpStatus::PartialContent);

  if (range.size() > 1) {
    // generate a multipart/byteranged response, as we've more than one range to
    // serve

    Buffer buf;
    std::string boundary(generateBoundaryID());
    std::size_t contentLength = 0;

    for (int i = 0, e = range.size(); i != e; ++i) {
      std::pair<std::size_t, std::size_t> offsets(
          makeOffsets(range[i], transferFile.size()));
      if (offsets.second < offsets.first) {
        response->setStatus(HttpStatus::PartialContent);
        response->completed();
        return true;
      }

      std::size_t partLength = 1 + offsets.second - offsets.first;

      buf.clear();
      buf.push_back("\r\n--");
      buf.push_back(boundary);
      buf.push_back("\r\nContent-Type: ");
      buf.push_back(transferFile.mimetype());

      buf.push_back("\r\nContent-Range: bytes ");
      buf.push_back(offsets.first);
      buf.push_back("-");
      buf.push_back(offsets.second);
      buf.push_back("/");
      buf.push_back(transferFile.size());
      buf.push_back("\r\n\r\n");

      contentLength += buf.size() + partLength;

      if (fd >= 0) {
        bool last = i + 1 == e;
        response->output()->write(std::move(buf));
        response->output()->write(FileRef(fd, offsets.first, partLength, last));
      }
    }

    buf.clear();
    buf.push_back("\r\n--");
    buf.push_back(boundary);
    buf.push_back("--\r\n");

    contentLength += buf.size();
    response->output()->write(std::move(buf));

    // push the prepared ranged response into the client
    response->setContentLength(contentLength);
    response->addHeader("Content-Type",
                        "multipart/byteranges; boundary=" + boundary);
  } else {  // generate a simple (single) partial response
    std::pair<std::size_t, std::size_t> offsets(
        makeOffsets(range[0], transferFile.size()));

    if (offsets.second < offsets.first) {
      response->setStatus(HttpStatus::RequestedRangeNotSatisfiable);
      response->completed();
      return true;
    }

    response->addHeader("Content-Type", transferFile.mimetype());

    std::size_t length = 1 + offsets.second - offsets.first;
    response->setContentLength(length);

    char cr[128];
    snprintf(cr, sizeof(cr), "bytes %zu-%zu/%zu", offsets.first, offsets.second,
             transferFile.size());
    response->addHeader("Content-Range", cr);

    if (fd >= 0) {
#if defined(HAVE_POSIX_FADVISE)
      posix_fadvise(fd, offsets.first, length, POSIX_FADV_SEQUENTIAL);
#endif
      response->output()->write(FileRef(fd, offsets.first, length, true));
    }
  }

  response->completed();
  return true;
}

} // namespace xzero
