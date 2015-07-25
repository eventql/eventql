// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/net/LocalDatagramConnector.h>
#include <cortex-base/net/LocalDatagramEndPoint.h>
#include <cortex-base/RuntimeError.h>
#include <cortex-base/logging.h>

namespace cortex {

LocalDatagramConnector::LocalDatagramConnector(
    const std::string& name,
    DatagramHandler handler,
    Executor* executor)
    : DatagramConnector(name, handler, executor),
      isStarted_(false),
      pending_() {
}

LocalDatagramConnector::~LocalDatagramConnector() {
}

void LocalDatagramConnector::start() {
  if (isStarted_)
    RAISE(IllegalStateError);

  isStarted_ = true;

  runQueue();
}

bool LocalDatagramConnector::isStarted() const {
  return isStarted_;
}

void LocalDatagramConnector::stop() {
  if (!isStarted_)
    RAISE(IllegalStateError);

  isStarted_ = false;
}

RefPtr<LocalDatagramEndPoint> LocalDatagramConnector::send(
    const BufferRef& message) {
  Buffer buf;
  buf.push_back(message);

  return send(std::move(buf));
}

RefPtr<LocalDatagramEndPoint> LocalDatagramConnector::send(Buffer&& message) {
  RefPtr<LocalDatagramEndPoint> ep(
      new LocalDatagramEndPoint(this, std::move(message)));

  pending_.push_back(ep);

  runQueue();

  return ep;
}

void LocalDatagramConnector::runQueue() {
  auto rq = std::move(pending_);

  if (!handler_) {
    logDebug("LocalDatagramConnector",
             "runQueue: Ignore pending messages. No handler set.");
    return;
  }

  while (!rq.empty()) {
    auto ep = rq.front();
    rq.pop_front();
    executor_->execute(std::bind(handler_, ep.as<DatagramEndPoint>()));
  }
}

} // namespace cortex
