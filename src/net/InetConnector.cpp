#include <xzero/executor/Executor.h>
#include <xzero/net/InetConnector.h>
#include <xzero/net/InetEndPoint.h>
#include <xzero/net/ConnectionFactory.h>
#include <xzero/net/Connection.h>
#include <xzero/net/IPAddress.h>
#include <xzero/io/SelectionKey.h>
#include <xzero/sysconfig.h>
#include <algorithm>
#include <mutex>
#include <system_error>

#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>

#if defined(SD_FOUND)
#include <systemd/sd-daemon.h>
#endif

#if !defined(SO_REUSEPORT)
#define SO_REUSEPORT 15
#endif

#if 0 // !defined(NDEBUG)
static std::mutex m;
#define TRACE(msg...)  do { \
    m.lock(); \
    printf("InetConnector: " msg); \
    printf("\n"); \
    m.unlock(); \
  } while (0);
#else
#define TRACE(msg...) do { } while (0)
#endif

namespace xzero {

InetConnector::InetConnector(const std::string& name, Executor* executor,
                             Scheduler* scheduler, Selector* selector)
    : Connector(name, executor),
      selector_(selector),
      scheduler_(scheduler),
      connectedEndPoints_(),
      socket_(-1),
      addressFamily_(IPAddress::V4),
      typeMask_(0),
      flags_(0),
      backlog_(256),
      multiAcceptCount_(1),
      isStarted_(false) {
}

InetConnector::InetConnector(const std::string& name, Executor* executor,
                             Scheduler* scheduler, Selector* selector,
                             const IPAddress& ipaddress, int port, int backlog,
                             bool reuseAddr, bool reusePort)
    : InetConnector(name, executor, scheduler, selector) {

  open(ipaddress, port, backlog, reuseAddr, reusePort);
}

void InetConnector::open(const IPAddress& ipaddress, int port, int backlog,
                         bool reuseAddr, bool reusePort) {
  if (isOpen())
    throw std::runtime_error("InetConnector already opened");

  socket_ = ::socket(ipaddress.family(), SOCK_STREAM, 0);
  addressFamily_ = ipaddress.family();

  if (socket_ < 0)
    throw std::system_error(errno, std::system_category());

  if (reusePort)
    setReusePort(reusePort);

  if (reuseAddr)
    setReuseAddr(reuseAddr);

  bind(ipaddress, port);
}

void InetConnector::close() {
  if (isStarted()) {
    stop();
  }

  if (isOpen()) {
    ::close(socket_);
    socket_ = -1;
  }
}

void InetConnector::bind(const IPAddress& ipaddr, int port) {
  char sa[sizeof(sockaddr_in6)];
  socklen_t salen = ipaddr.size();

  switch (ipaddr.family()) {
    case IPAddress::V4:
      salen = sizeof(sockaddr_in);
      ((sockaddr_in*)sa)->sin_port = htons(port);
      ((sockaddr_in*)sa)->sin_family = AF_INET;
      memcpy(&((sockaddr_in*)sa)->sin_addr, ipaddr.data(), ipaddr.size());
      break;
    case IPAddress::V6:
      salen = sizeof(sockaddr_in6);
      ((sockaddr_in6*)sa)->sin6_port = htons(port);
      ((sockaddr_in6*)sa)->sin6_family = AF_INET6;
      memcpy(&((sockaddr_in6*)sa)->sin6_addr, ipaddr.data(), ipaddr.size());
      break;
    default:
      throw std::system_error(EINVAL, std::system_category());
  }

  int rv = ::bind(socket_, (sockaddr*)sa, salen);
  if (rv < 0)
    throw std::system_error(errno, std::system_category());

  addressFamily_ = ipaddr.family();
}

void InetConnector::listen(int backlog) {
  int rv = ::listen(socket_, backlog);
  if (rv < 0)
    throw std::system_error(errno, std::system_category());
}

bool InetConnector::isOpen() const noexcept {
  return socket_ >= 0;
}

InetConnector::~InetConnector() {
  TRACE("~InetConnector");
  if (isStarted()) {
    stop();
  }

  if (socket_ >= 0) {
    ::close(socket_);
  }
}

Selector* InetConnector::selector() const noexcept {
  return selector_;
}

int InetConnector::handle() const noexcept {
  return socket_;
}

void InetConnector::setSocket(int socket) {
  socket_ = socket;
}

size_t InetConnector::backlog() const noexcept {
  return backlog_;
}

void InetConnector::setBacklog(size_t value) {
  if (isStarted()) {
    throw std::runtime_error("Cannot change backlog when already listening.");
  }

  backlog_ = value;
}

bool InetConnector::isBlocking() const {
  return !(fcntl(socket_, F_GETFL) & O_NONBLOCK);
}

void InetConnector::setBlocking(bool enable) {
  unsigned flags = enable ? fcntl(socket_, F_GETFL) & ~O_NONBLOCK
                          : fcntl(socket_, F_GETFL) | O_NONBLOCK;

  if (fcntl(socket_, F_SETFL, flags) < 0) {
    throw std::system_error(errno, std::system_category());
  }

#if defined(HAVE_ACCEPT4) && defined(ENABLE_ACCEPT4)
  if (enable) {
    typeMask_ |= SOCK_NONBLOCK;
  } else {
    typeMask_ &= ~SOCK_NONBLOCK;
  }
#else
  if (enable) {
    flags_ |= O_NONBLOCK;
  } else {
    flags_ &= ~O_NONBLOCK;
  }
#endif
}

bool InetConnector::closeOnExec() const {
  return fcntl(socket_, F_GETFD) & FD_CLOEXEC;
}

void InetConnector::setCloseOnExec(bool enable) {
  unsigned flags = enable ? fcntl(socket_, F_GETFD) | FD_CLOEXEC
                          : fcntl(socket_, F_GETFD) & ~FD_CLOEXEC;

  if (fcntl(socket_, F_SETFD, flags) < 0) {
    throw std::system_error(errno, std::system_category());
  }

#if defined(HAVE_ACCEPT4) && defined(ENABLE_ACCEPT4)
  if (enable) {
    typeMask_ |= SOCK_CLOEXEC;
  } else {
    typeMask_ &= ~SOCK_CLOEXEC;
  }
#else
  if (enable) {
    flags_ |= O_CLOEXEC;
  } else {
    flags_ &= ~O_CLOEXEC;
  }
#endif
}

bool InetConnector::deferAccept() const {
  int optval = 1;
  socklen_t optlen = sizeof(optval);
  return ::getsockopt(socket_, SOL_TCP, TCP_DEFER_ACCEPT, &optval, &optlen) == 0
             ? optval != 0
             : false;
}

void InetConnector::setDeferAccept(bool enable) {
#if defined(TCP_DEFER_ACCEPT) && defined(ENABLE_TCP_DEFER_ACCEPT)
  int rc = enable ? 1 : 0;
  if (::setsockopt(socket_, SOL_TCP, TCP_DEFER_ACCEPT, &rc, sizeof(rc)) < 0) {
    throw std::system_error(errno, std::system_category());
  }
#else
  throw std::system_error(ENOTSUP, std::system_category());
#endif
}

bool InetConnector::quickAck() const {
  int optval = 1;
  socklen_t optlen = sizeof(optval);
  return ::getsockopt(socket_, SOL_TCP, TCP_QUICKACK, &optval, &optlen) == 0
             ? optval != 0
             : false;
}

void InetConnector::setQuickAck(bool enable) {
  int rc = enable ? 1 : 0;
  if (::setsockopt(socket_, SOL_TCP, TCP_QUICKACK, &rc, sizeof(rc)) < 0) {
    throw std::system_error(errno, std::system_category());
  }
}

bool InetConnector::reusePort() const {
  int optval = 1;
  socklen_t optlen = sizeof(optval);
  return ::getsockopt(socket_, SOL_SOCKET, SO_REUSEPORT, &optval, &optlen) == 0
             ? optval != 0
             : false;
}

void InetConnector::setReusePort(bool enable) {
  int rc = enable ? 1 : 0;
  if (::setsockopt(socket_, SOL_SOCKET, SO_REUSEPORT, &rc, sizeof(rc)) < 0) {
    throw std::system_error(errno, std::system_category());
  }
}

bool InetConnector::reuseAddr() const {
  int optval = 1;
  socklen_t optlen = sizeof(optval);
  return ::getsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &optval, &optlen) == 0
             ? optval != 0
             : false;
}

void InetConnector::setReuseAddr(bool enable) {
  int rc = enable ? 1 : 0;
  if (::setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &rc, sizeof(rc)) < 0) {
    throw std::system_error(errno, std::system_category());
  }
}

size_t InetConnector::multiAcceptCount() const noexcept {
  return multiAcceptCount_;
}

void InetConnector::setMultiAcceptCount(size_t value) noexcept {
  multiAcceptCount_ = value;
}

void InetConnector::start() {
  if (!isOpen()) {
    throw std::runtime_error("Connector must be open in order to be started.");
  }

  if (isStarted()) {
    return;
  }

  listen(backlog_);

  isStarted_ = true;

  if (selector_)
    selectionKey_ = selector_->createSelectable(this, READ);
  else {
    executor()->execute([this]() {
      while (isStarted_) {
        acceptOne();
      }
    });
  }
}

bool InetConnector::isStarted() const noexcept {
  return isStarted_;
}

void InetConnector::stop() {
  TRACE("stop: %s", name().c_str());
  if (!isStarted()) {
    return;
  }

  selectionKey_.reset();
  isStarted_ = false;
}

void InetConnector::onSelectable() noexcept {
  try {
    for (size_t i = 0; i < multiAcceptCount_; i++) {
      if (!acceptOne()) {
        break;
      }
    }
  } catch (const std::exception& e) {
    // TODO
  } catch (...) {
    // TODO
  }
}

bool InetConnector::acceptOne() {
#if defined(HAVE_ACCEPT4) && defined(ENABLE_ACCEPT4)
  bool flagged = true;
  int cfd = ::accept4(socket_, nullptr, 0, typeMask_);
  if (cfd < 0 && errno == ENOSYS) {
    cfd = ::accept(socket_, nullptr, 0);
    flagged = false;
  }
#else
  bool flagged = false;
  int cfd = ::accept(socket_, nullptr, 0);
#endif

  if (cfd < 0) {
    switch (errno) {
      case EINTR:
      case EAGAIN:
#if EAGAIN != EWOULDBLOCK
      case EWOULDBLOCK:
#endif
        return true;
      default:
        throw std::system_error(errno, std::system_category());
    }
  }

  if (!flagged && flags_ &&
      fcntl(cfd, F_SETFL, fcntl(cfd, F_GETFL) | flags_) < 0) {
    ::close(cfd);
    throw std::system_error(errno, std::system_category());
  }

  std::unique_ptr<InetEndPoint> endpoint(new InetEndPoint(cfd, this));
  connectedEndPoints_.push_back(endpoint.get());

  auto connection =
      defaultConnectionFactory()->create(this, std::move(endpoint));
  connection->onOpen();

  return true;
}

std::list<EndPoint*> InetConnector::connectedEndPoints() {
  std::list<EndPoint*> result;
  for (InetEndPoint* ep : connectedEndPoints_) {
    result.push_back(ep);
  }
  return result;
}

void InetConnector::onEndPointClosed(InetEndPoint* endpoint) {
  assert(endpoint != nullptr);
  assert(endpoint->connection() != nullptr);

  auto i = std::find(connectedEndPoints_.begin(), connectedEndPoints_.end(),
                     endpoint);

  assert(i != connectedEndPoints_.end());

  if (i != connectedEndPoints_.end()) {
    endpoint->connection()->onClose();

    connectedEndPoints_.erase(i);

    if (!endpoint->isBusy()) {
      // do only actually delete if not currently inside an io handler
      release(endpoint->connection());
    }
  }
}

void InetConnector::release(Connection* inetConnection) {
  assert(inetConnection != nullptr);
  assert(dynamic_cast<InetEndPoint*>(inetConnection->endpoint()) != nullptr);
  assert(!static_cast<InetEndPoint*>(inetConnection->endpoint())->isBusy());

  delete inetConnection;
}

}  // namespace xzero
