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
#include <cortex-base/cli/FlagType.h>
#include <functional>
#include <vector>
#include <string>
#include <unordered_map>
#include <utility>

namespace cortex {

class IPAddress;

// FlagPassingStyle
enum FlagStyle {
  ShortSwitch,
  LongSwitch,
  ShortWithValue,
  LongWithValue,
  UnnamedParameter
};

class CORTEX_API Flag {
 public:
  Flag(
      const std::string& opt,
      const std::string& val,
      FlagStyle fs,
      FlagType ft);

  explicit Flag(char shortOpt);
  Flag(char shortOpt, const std::string& val);
  Flag(const std::string& longOpt);
  Flag(const std::string& longOpt, const std::string& val);

  FlagType type() const { return type_; }
  const std::string& name() const { return name_; }
  const std::string& value() const { return value_; }

 private:
  FlagType type_;
  FlagStyle style_;
  std::string name_;
  std::string value_;
};

typedef std::pair<FlagType, std::string> FlagValue;

class CORTEX_API Flags {
 public:
  Flags();

  void merge(const std::vector<Flag>& args);
  void merge(const Flag& flag);

  template<typename... Args>
  void set(Args... args) { merge(Flag(args...)); }
  bool isSet(const std::string& flag) const;

  IPAddress getIPAddress(const std::string& flag) const;
  std::string getString(const std::string& flag) const;
  std::string asString(const std::string& flag) const;
  long int getNumber(const std::string& flag) const;
  float getFloat(const std::string& flag) const;
  bool getBool(const std::string& flag) const;

  const std::vector<std::string>& parameters() const;
  void setParameters(const std::vector<std::string>& v);

  size_t size() const { return set_.size(); }

  std::string to_s() const;

 private:
  std::unordered_map<std::string, FlagValue> set_;
  std::vector<std::string> raw_;
};

// maybe via CLI / FlagBuilder
CORTEX_API std::string inspect(const Flags& flags);

}  // namespace cortex
