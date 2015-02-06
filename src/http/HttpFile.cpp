// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/http/HttpFile.h>
#include <xzero/Buffer.h>
#include <xzero/sysconfig.h>

#include <ctime>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

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
  size_t n = path_.rfind('/');
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

} // namespace xzero
