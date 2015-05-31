/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-dproc/TaskResult.h>

namespace fnord {
namespace dproc {

Future<RefPtr<VFSFile>> TaskResult::result() const {
  return promise_.future();
}

void TaskResult::returnResult(RefPtr<VFSFile> result) {
  promise_.success(result);
}

void TaskResult::returnError(const StandardException& e) {
  promise_.failure(e);
}

} // namespace dproc
} // namespace fnord
