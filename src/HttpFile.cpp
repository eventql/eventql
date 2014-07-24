// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/HttpFile.h>
#include <xzero/HttpFileMgr.h>
#include <base/DebugLogger.h>
#include <base/Buffer.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#define TRACE(level, msg...) XZERO_DEBUG("HttpFile", (level), msg)

namespace xzero {

HttpFile::HttpFile(const std::string& path, HttpFileMgr* mgr)
    : mgr_(mgr),
      path_(path),
      fd_(-1),
      stat_(),
      refs_(0),
      errno_(0),
#if defined(HAVE_SYS_INOTIFY_H)
      inotifyId_(-1),
#endif
      cachedAt_(0),
      etag_(),
      mtime_(),
      mimetype_() {
  TRACE(2, "(%s).new", path_.c_str());
  open();
}

HttpFile::~HttpFile() {
  clearCache();

  close();
  TRACE(2, "(%s).destruct", path_.c_str());
}

std::string HttpFile::filename() const {
  std::size_t n = path_.rfind('/');

  return n != std::string::npos ? path_.substr(n + 1) : path_;
}

bool HttpFile::open() {
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

  fd_ = ::open(path_.c_str(), flags);
  if (fd_ < 0) {
    errno_ = errno;
    return false;
  }

  return update();
}

bool HttpFile::update() {
  int rv = fstat(fd_, &stat_);
  if (rv < 0) {
    errno_ = errno;
    return false;
  }

  cachedAt_ = ev_now(mgr_->loop_);

  return true;
}

void HttpFile::close() {
  if (fd_ >= 0) {
    TRACE(1, "(%s).close() fd:%d", path_.c_str(), fd_);
    ::close(fd_);
    fd_ = -1;
  }
}

void HttpFile::ref() {
  ++refs_;

  TRACE(2, "(%s).ref() -> %i", path_.c_str(), refs_);
}

void HttpFile::unref() {
  --refs_;

  TRACE(2, "(%s).unref() -> %i", path_.c_str(), refs_);

  if (refs_ == 1)
    mgr_->release(this);
  else if (refs_ <= 0)
    delete this;
}

void HttpFile::clearCache() {
  clearCustomData();
  etag_.clear();
  mtime_.clear();
  // do not clear mimetype, as this didn't change
}

bool HttpFile::isValid() const {
#if defined(HAVE_SYS_INOTIFY_H)
  return inotifyId_ > 0 ||
         cachedAt_ + mgr_->settings_->cacheTTL > ev_now(mgr_->loop_);
#else
  return cachedAt_ + mgr_->settings_->cacheTTL > ev_now(mgr_->loop_);
#endif
}

const std::string& HttpFile::lastModified() const {
  if (mtime_.empty()) {
    if (struct tm* tm = std::gmtime(&stat_.st_mtime)) {
      char buf[256];

      if (std::strftime(buf, sizeof(buf), "%a, %d %b %Y %T GMT", tm) != 0) {
        mtime_ = buf;
      }
    }
  }

  return mtime_;
}

const std::string& HttpFile::etag() const {
  // compute entity tag
  if (etag_.empty()) {
    size_t count = 0;
    Buffer buf(256);
    buf.push_back('"');

    if (mgr_->settings_->etagConsiderMtime) {
      if (count++) buf.push_back('-');
      buf.push_back(stat_.st_mtime);
    }

    if (mgr_->settings_->etagConsiderSize) {
      if (count++) buf.push_back('-');
      buf.push_back(stat_.st_size);
    }

    if (mgr_->settings_->etagConsiderInode) {
      if (count++) buf.push_back('-');
      buf.push_back(stat_.st_ino);
    }

    buf.push_back('"');

    etag_ = buf.str();
  }

  return etag_;
}

const std::string& HttpFile::mimetype() const {
  if (!mimetype_.empty()) return mimetype_;

  // query mimetype
  std::size_t ndot = path_.find_last_of(".");
  std::size_t nslash = path_.find_last_of("/");

  if (ndot != std::string::npos && ndot > nslash) {
    const auto& mimetypes = mgr_->settings_->mimetypes;
    std::string ext(path_.substr(ndot + 1));

    mimetype_.clear();

    while (ext.size()) {
      auto i = mimetypes.find(ext);

      if (i != mimetypes.end()) {
        // DEBUG("filename(%s), ext(%s), use mimetype: %s", path_.c_str(),
        // ext.c_str(), i->second.c_str());
        mimetype_ = i->second;
      }

      if (ext[ext.size() - 1] != '~') break;

      ext.resize(ext.size() - 1);
    }

    if (mimetype_.empty()) {
      mimetype_ = mgr_->settings_->defaultMimetype;
    }
  } else {
    mimetype_ = mgr_->settings_->defaultMimetype;
  }

  return mimetype_;
}

}  // namespace xzero
