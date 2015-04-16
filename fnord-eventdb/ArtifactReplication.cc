/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "unistd.h"
#include <fnord-base/logging.h>
#include <fnord-base/wallclock.h>
#include <fnord-http/HTTPFileDownload.h>
#include <fnord-eventdb/ArtifactReplication.h>

namespace fnord {
namespace eventdb {

ArtifactReplication::ArtifactReplication(
    ArtifactIndex* index,
    http::HTTPConnectionPool* http) :
    index_(index),
    http_(http),
    interval_(1 * kMicrosPerSecond),
    cur_requests_(0),
    max_concurrent_reqs_(4),
    running_(true) {}

void ArtifactReplication::addSource(const URI& source) {
  sources_.emplace_back(source);
}

void ArtifactReplication::downloadPending() {
  URI uri("http://nue03.prod.fnrd.net:7005/chunks/dawanda_joined_sessions.nue03.db3203a33a8df903028d87658a5eb502.sst");
  http::HTTPRequest req(http::HTTPMessage::M_GET, uri.pathAndQuery());
  req.addHeader("Host", uri.hostAndPort());
  http::HTTPFileDownload download(req, "/tmp/fu.download");
  auto res = download.download(http_);
  res.wait();
  fnord::iputs("status: $0", res.get().statusCode());
  abort();

  auto artifacts = index_->listArtifacts();

  for (const auto& a : artifacts) {
    if (a.status == ArtifactStatus::DOWNLOAD) {
      enqueueArtifact(a);
    }
  }
}

void ArtifactReplication::downloadArtifact(const ArtifactRef& artifact) {
  fnord::logInfo(
      "fnord.evdb",
      "Downloading artifact: $0",
      artifact.name);

  // here be dragons...

  //http://nue03.prod.fnrd.net:7005/chunks/

  for (const auto& f : artifact.files) {

    for (const auto& s : sources_) {
      auto path = FileUtil::joinPaths(s.path(), f.filename);
      http::HTTPRequest req(http::HTTPMessage::M_HEAD, path);
      req.addHeader("Host", s.hostAndPort());

      fnord::iputs("GET: $0, $1", s.hostAndPort(), path);
      auto res = http_->executeRequest(req);
      res.wait();

      const auto& r = res.get();
      fnord::iputs("response: $0", r.statusCode());
      //if (r.statusCode() != 201) {
      //  RAISEF(kRuntimeError, "received non-201 response: $0", r.body().toString());
      //}

    }
  }

  index_->updateStatus(artifact.name, ArtifactStatus::PRESENT);
}

void ArtifactReplication::start() {
  running_ = true;
  thread_ = std::thread(std::bind(&ArtifactReplication::run, this));
}

void ArtifactReplication::stop() {
  if (!running_) {
    return;
  }

  running_ = false;
  thread_.join();
}

void ArtifactReplication::enqueueArtifact(const ArtifactRef& artifact) {
  std::unique_lock<std::mutex> lk(mutex_);
  while (cur_requests_ >= max_concurrent_reqs_) {
    cv_.wait(lk);
  }

  ++cur_requests_;
  auto thread = std::thread([this, artifact] () {
    downloadArtifact(artifact);
    std::unique_lock<std::mutex> wakeup_lk(mutex_);
    --cur_requests_;
    wakeup_lk.unlock();
    cv_.notify_all();
  });

  thread.detach();
}

void ArtifactReplication::run() {
  while (running_.load()) {
    auto begin = WallClock::unixMicros();

    try {
      downloadPending();
    } catch (const Exception& e) {
      fnord::logError("fnord.evdb", e, "ArtifactReplication error");
    }

    auto elapsed = WallClock::unixMicros() - begin;
    if (elapsed < interval_) {
      usleep(interval_ - elapsed);
    }
  }
}

} // namespace eventdb
} // namespace fnord

