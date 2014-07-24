// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/HttpFileMgr.h>
#include <base/Buffer.h>
#include <base/Tokenizer.h>
#include <base/DebugLogger.h>
#include <stdio.h>
#include <errno.h>

#if 1
#define TRACE(level, msg...) XZERO_DEBUG("HttpFileMgr", (level), msg)
#else
#define TRACE(level, msg...) \
  do {                       \
  } while (0)
#endif

namespace xzero {

HttpFileMgr::HttpFileMgr(struct ev_loop* loop, Settings* settings)
    : loop_(loop),
#if defined(HAVE_SYS_INOTIFY_H)
      handle_(-1),
      inotify_(loop_),
      inotifies_(),
#endif
      settings_(settings),
      cache_() {
#if defined(HAVE_SYS_INOTIFY_H)
  handle_ = inotify_init();
  if (handle_ != -1) {
    if (fcntl(handle_, F_SETFL, fcntl(handle_, F_GETFL) | O_NONBLOCK) < 0)
      fprintf(stderr,
              "Error setting nonblock/cloexec flags on inotify handle\n");

    if (fcntl(handle_, F_SETFD, fcntl(handle_, F_GETFD) | FD_CLOEXEC) < 0)
      fprintf(stderr, "Error setting cloexec flags on inotify handle\n");

    inotify_.set<HttpFileMgr, &HttpFileMgr::onFileChanged>(this);
    inotify_.start(handle_, ev::READ);
    loop_.unref();
    TRACE(1, "inotify handle=%d\n", handle_);
  } else {
    fprintf(stderr, "Error initializing inotify: %s\n", strerror(errno));
  }
#endif
}

HttpFileMgr::~HttpFileMgr() {
#if defined(HAVE_SYS_INOTIFY_H)
  TRACE(1, "~HttpFileMgr (inotify=%d)\n", handle_);
#endif
  stop();
}

void HttpFileMgr::stop() {
#if defined(HAVE_SYS_INOTIFY_H)
  TRACE(1, "close() (inotify=%d)\n", handle_);
  if (handle_ != -1) {
    loop_.ref();
    inotify_.stop();
    ::close(handle_);
    handle_ = -1;
  }
#endif
}

HttpFileRef HttpFileMgr::query(const BufferRef& path) {
  return query(path.str());
}

HttpFileRef HttpFileMgr::query(const std::string& path) {
  auto i = cache_.find(path);
  if (i != cache_.end()) {
    TRACE(1, "query(%s).cached; size:%ld\n", path.c_str(), i->second->size());
    return i->second;
  }

  //	return cache_[path] = HttpFileRef(new HttpFile(path, this));

  auto file = HttpFileRef(new HttpFile(path, this));

#if defined(HAVE_SYS_INOTIFY_H)
  int wd =
      handle_ != -1 && file->exists()
          ? ::inotify_add_watch(handle_, path.c_str(),
                                /*IN_ONESHOT |*/ IN_ATTRIB | IN_DELETE_SELF |
                                    IN_MOVE_SELF | IN_UNMOUNT | IN_DELETE_SELF |
                                    IN_MOVE_SELF)
          : -1;

  TRACE(1, "query(%s).new -> %d len:%ld\n", path.c_str(), wd, file->size());

  if (wd != -1) {
    file->inotifyId_ = wd;
    inotifies_[wd] = file.get();
    cache_[path] = file;
  }
#else
  TRACE(1, "query(%s)! len:%ld\n", path.c_str(), file->size());
  cache_[path] = file;
#endif
  return file;
}

static inline bool readFile(const std::string& path, Buffer* output) {
  int fd = open(path.c_str(), O_RDONLY);
  if (fd < 0) return false;

  struct stat st;
  if (fstat(fd, &st) < 0) return false;

  output->reserve(st.st_size + 1);
  ssize_t nread = ::read(fd, output->data(), st.st_size);
  if (nread < 0) {
    ::close(fd);
    return false;
  }

  output->data()[nread] = '\0';
  output->resize(nread);
  ::close(fd);

  return true;
}

bool HttpFileMgr::Settings::openMimeTypes(const std::string& path) {
  Buffer input;
  if (!readFile(path, &input)) return false;

  auto lines = Tokenizer<BufferRef, Buffer>::tokenize(input, "\n");

  mimetypes.clear();

  for (auto line : lines) {
    line = line.trim();
    auto columns = Tokenizer<BufferRef, BufferRef>::tokenize(line, " \t");

    auto ci = columns.begin(), ce = columns.end();
    BufferRef mime = ci != ce ? *ci++ : BufferRef();

    if (!mime.empty() && mime[0] != '#') {
      for (; ci != ce; ++ci) {
        mimetypes[ci->str()] = mime.str();
      }
    }
  }

  return true;
}

void HttpFileMgr::release(HttpFile* file) {
  TRACE(1, "release(%p, %s) rc:%d\n", file, file->path().c_str(), file->refs_);
#if defined(HAVE_SYS_INOTIFY_H)
  auto wi = inotifies_.find(file->inotifyId_);
  if (wi != inotifies_.end()) inotifies_.erase(wi);
#endif

  auto k = cache_.find(file->path());
  if (k != cache_.end()) cache_.erase(k);
}

#if defined(HAVE_SYS_INOTIFY_H)
void HttpFileMgr::onFileChanged(ev::io& w, int revents) {
  TRACE(1, "onFileChanged()\n");

  char buf[sizeof(inotify_event) * 256];
  ssize_t rv = ::read(handle_, &buf, sizeof(buf));

  TRACE(2, "read returned: %ld (%% %ld, %ld)\n", rv, sizeof(inotify_event),
        rv / sizeof(inotify_event));

  if (rv > 0) {
    const char* i = buf;
    const char* e = i + rv;
    inotify_event* ev = (inotify_event*)i;

    for (; i < e && ev->wd != 0;
         i += sizeof(*ev) + ev->len, ev = (inotify_event*)i) {
      TRACE(2, "traverse: (wd:%d, mask:0x%04x, cookie:%d)\n", ev->wd, ev->mask,
            ev->cookie);
      auto wi = inotifies_.find(ev->wd);
      if (wi == inotifies_.end()) {
        TRACE(3, "-skipping\n");
        continue;
      }

      auto k = cache_.find(wi->second->path());
      TRACE(1, "invalidate: %s\n", k->first.c_str());
      // onInvalidate(k->first, k->second);
      inotifies_.erase(wi);
      cache_.erase(k);
      int rv = inotify_rm_watch(handle_, ev->wd);
      if (rv < 0) {
        TRACE(1, "error removing inotify watch (%d, %s): %s\n", ev->wd,
              ev->name, strerror(errno));
      } else {
        TRACE(2, "inotify_rm_watch: %d (ok)\n", rv);
      }
    }
  }
}
#endif
}  // namespace xzero
