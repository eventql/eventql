/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/cli/CLICommand.h>

namespace stx {
namespace cli {

CLICommand::CLICommand(const String& command) {}

void CLICommand::onCall(CallFnType fn) {
  on_call_ = fn;
}

void CLICommand::call(const Vector<String>& argv) {
  flags_.parseArgv(argv);
  on_call_(flags_);
}

FlagParser& CLICommand::flags() {
  return flags_;
}

}
}
