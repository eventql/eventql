/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_DPROC_APPLICATION_H
#define _FNORD_DPROC_APPLICATION_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/autoref.h>
#include <fnord-dproc/Task.h>

namespace fnord {
namespace dproc {

class Application {
public:

  void getTaskInstance(const String& name, const Buffer& params);

  void registerTaskFactory(const String& name, TaskFactory factory);

};

} // namespace cstable
} // namespace fnord

#endif
