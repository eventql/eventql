// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-base/cli/Flags.h>
#include <cortex-base/cli/CLI.h>
#include <cortex-base/net/IPAddress.h>
#include <cortex-base/RuntimeError.h>
#include <cortex-base/Buffer.h>
#include <sstream>

extern char** environ;

namespace cortex {

// {{{ Flag
Flag::Flag(
    const std::string& opt,
    const std::string& val,
    FlagStyle fs,
    FlagType ft)
    : type_(ft),
      style_(fs),
      name_(opt),
      value_(val) {
}
// }}}

Flags::Flags() {
}

void Flags::merge(const std::vector<Flag>& args) {
  for (const Flag& arg: args)
    merge(arg);
}

void Flags::merge(const Flag& flag) {
  set_[flag.name()] = std::make_pair(flag.type(), flag.value());
}

bool Flags::isSet(const std::string& flag) const {
  return set_.find(flag) != set_.end();
}

IPAddress Flags::getIPAddress(const std::string& flag) const {
  auto i = set_.find(flag);
  if (i == set_.end())
    RAISE_STATUS(CliFlagNotFoundError);

  if (i->second.first != FlagType::IP)
    RAISE_STATUS(CliTypeMismatchError);

  return IPAddress(i->second.second);
}

std::string Flags::asString(const std::string& flag) const {
  auto i = set_.find(flag);
  if (i == set_.end())
    RAISE_STATUS(CliFlagNotFoundError);

  return i->second.second;
}

std::string Flags::getString(const std::string& flag) const {
  auto i = set_.find(flag);
  if (i == set_.end())
    RAISE_STATUS(CliFlagNotFoundError);

  if (i->second.first != FlagType::String)
    RAISE_STATUS(CliTypeMismatchError);

  return i->second.second;
}

long int Flags::getNumber(const std::string& flag) const {
  auto i = set_.find(flag);
  if (i == set_.end())
    RAISE_STATUS(CliFlagNotFoundError);

  if (i->second.first != FlagType::Number)
    RAISE_STATUS(CliTypeMismatchError);

  return std::stoi(i->second.second);
}

float Flags::getFloat(const std::string& flag) const {
  auto i = set_.find(flag);
  if (i == set_.end())
    RAISE_STATUS(CliFlagNotFoundError);

  if (i->second.first != FlagType::Float)
    RAISE_STATUS(CliTypeMismatchError);

  return std::stof(i->second.second);
}

bool Flags::getBool(const std::string& flag) const {
  auto i = set_.find(flag);
  if (i == set_.end())
    return false;

  return i->second.second == "true";
}

const std::vector<std::string>& Flags::parameters() const {
  return raw_;
}

void Flags::setParameters(const std::vector<std::string>& v) {
  raw_ = v;
}

std::string Flags::to_s() const {
  std::stringstream sstr;

  int i = 0;
  for (const auto& flag: set_) {
    if (i)
      sstr << ' ';

    i++;

    switch (flag.second.first) {
      case FlagType::Bool:
        if (flag.second.second == "true")
          sstr << "--" << flag.first;
        else
          sstr << "--" << flag.first << "=false";
        break;
      case FlagType::String:
        sstr << "--" << flag.first << "=\"" << flag.second.second << "\"";
        break;
      default:
        sstr << "--" << flag.first << "=" << flag.second.second;
        break;
    }
  }

  return sstr.str();
}

std::string inspect(const Flags& flags) {
  return flags.to_s();
}

}  // namespace cortex
