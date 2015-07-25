// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
#pragma once

#include <cortex-base/Api.h>
#include <list>
#include <algorithm>
#include <functional>

namespace cortex {

//! \addtogroup cortex
//@{

/**
 * Multi channel signal API.
 */
template <typename SignatureT>
class Signal;

/**
 * @brief Signal API
 *
 * This API is based on the idea of Qt's signal/slot API.
 * You can connect zero or more callbacks to this signal that get invoked
 * sequentially when this signal is fired.
 */
template <typename... Args>
class Signal<void(Args...)> {
  Signal(const Signal&) = delete;
  Signal& operator=(const Signal&) = delete;

 private:
  typedef std::list<std::function<void(Args...)>> list_type;

 public:
  typedef typename list_type::iterator Connection;

 public:
  Signal() = default;
  Signal(Signal&&) = default;
  Signal& operator=(Signal&&) = default;

  /**
   * Tests whether this signal contains any listeners.
   */
  bool empty() const { return listeners_.empty(); }

  /**
   * Retrieves the number of listeners to this signal.
   */
  std::size_t size() const { return listeners_.size(); }

  /**
   * Connects a listener with this signal.
   */
  template <class K, void (K::*method)(Args...)>
  Connection connect(K* object) {
    return connect([=](Args... args) {
      (static_cast<K*>(object)->*method)(args...);
    });
  }

  /**
   * @brief Connects a listener with this signal.
   *
   * @return a handle to later explicitely disconnect from this signal again.
   */
  Connection connect(std::function<void(Args...)> cb) {
    listeners_.push_back(std::move(cb));
    auto handle = listeners_.end();
    --handle;
    return handle;
  }

  /**
   * @brief Disconnects a listener from this signal.
   */
  void disconnect(Connection c) { listeners_.erase(c); }

  /**
   * @brief invokes all listeners with the given args
   *
   * Triggers this signal by notifying all listeners via their registered
   * callback each with the given arguments passed.
   */
  void fire(const Args&... args) const {
    for (auto listener : listeners_) {
      listener(args...);
    }
  }

  /**
   * @brief invokes all listeners with the given args
   *
   * Triggers this signal by notifying all listeners via their registered
   * callback each with the given arguments passed.
   */
  void operator()(Args... args) const {
    fire(std::forward<Args>(args)...);
  }

  /**
   * Clears all listeners to this signal.
   */
  void clear() { listeners_.clear(); }

 private:
  list_type listeners_;
};

//@}

}  // namespace cortex
