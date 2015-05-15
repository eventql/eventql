/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-dproc/Application.h>

namespace fnord {
namespace dproc {

Application::Application(const String& name) : name_(name) {}

//void Application::getTaskInstance(const String& name, const Buffer& params);
//void Application::registerTaskFactory(const String& name, TaskFactory factory);

} // namespace dproc
} // namespace fnord

