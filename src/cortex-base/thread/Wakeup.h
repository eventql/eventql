// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/Api.h>
#include <condition_variable>
#include <atomic>
#include <mutex>
#include <list>

namespace cortex {

/**
 * Provides a facility to wait for events.
 *
 * While one or more caller are waiting for one event, another
 * caller can cause those waiting callers to be fired.
 */
class CORTEX_API Wakeup {
 public:
  Wakeup();

  /**
   * Block the current thread and wait for the next wakeup event.
   */
  void waitForNextWakeup();

  /**
   * Block the current thread and wait for the first wakeup event (generation 0).
   */
  void waitForFirstWakeup();

  /**
   * Block the current thread and wait for wakeup of generation > given.
   *
   * @param generation the generation number considered <b>old</b>.
   *                   Any generation number bigger this will wakeup the caller.
   */
  void waitForWakeup(long generation);

  /**
   * Increments the generation and invokes all waiters.
   */
  void wakeup();

  /**
   * Registeres a callback to be invoked when given generation has become old.
   *
   * @param generation Threshold of old generations.
   * @param callback Callback to invoke upon fire.
   */
  void onWakeup(long generation, std::function<void()> callback);

  /**
   * Retrieves the current wakeup-generation number.
   */
  long generation() const;

 protected:
  std::mutex mutex_;
  std::condition_variable condvar_;
  std::atomic<long> gen_;
  std::list<std::function<void()>> callbacks_;
};

} // namespace cortex
