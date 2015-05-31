/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_DPROC_TASKRESULT_H
#define _FNORD_DPROC_TASKRESULT_H
#include <fnord-base/stdtypes.h>
#include <fnord-dproc/Task.h>

namespace fnord {
namespace dproc {

class TaskResult : public RefCounted {
public:

  Future<RefPtr<VFSFile>> result() const;

  void returnResult(RefPtr<VFSFile> result);
  void returnError(const StandardException& e);

protected:
  Promise<RefPtr<VFSFile>> promise_;
};

} // namespace dproc
} // namespace fnord

#endif
