// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <stx/sysconfig.h>
#include <stx/executor/Executor.h>
#include <deque>

namespace stx {

/**
 * Executor to directly invoke the tasks being passed by the caller.
 *
 * @note Not thread-safe.
 */
class DirectExecutor : public Executor {
 public:
  DirectExecutor(
    bool recursive = false,
    std::unique_ptr<stx::ExceptionHandler> eh = nullptr);

  void execute(Task task) override;
  std::string toString() const override;

  /** Tests whether this executor is currently running some task. */
  bool isRunning() const { return running_ > 0; }

  /** Tests whether this executor allows recursive execution of tasks. */
  bool isRecursive() const { return recursive_; }

  /** Sets wether recursive execution is allowed or flattened. */
  void setRecursive(bool value) { recursive_ = value; }

  /** Retrieves number of deferred tasks. */
  size_t backlog() const { return deferred_.size(); }

 private:
  bool recursive_;
  int running_;
  std::deque<Task> deferred_;
};

} // namespace stx
