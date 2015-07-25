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
#include <cortex-base/RuntimeError.h>
#include <cortex-base/cli/FlagType.h>
#include <functional>
#include <list>
#include <vector>
#include <string>

namespace cortex {

class IPAddress;
class Flags;
class Flag;

/**
 * CLI - Command Line Interface.
 *
 * It reads program options from:
 * <ul>
 *   <li>environment variables, as well as</li>
 *   <li>program parameters</li>
 * </ul>
 */
class CORTEX_API CLI {
 public:
  struct FlagDef;
  class TypeMismatchError;
  class UnknownOptionError;
  class MissingOptionError;
  class MissingOptionValueError;

  CLI();

  // required string flag
  CLI& defineString(
      const std::string& longOpt,
      char shortOpt,
      const std::string& valuePlaceholder,
      const std::string& helpText,
      std::function<void(const std::string&)> callback = nullptr);

  // defaulted string flag
  CLI& defineString(
      const std::string& longOpt,
      char shortOpt,
      const std::string& valuePlaceholder,
      const std::string& helpText,
      const std::string& defaultValue,
      std::function<void(const std::string&)> callback = nullptr);

  // required number flag
  CLI& defineNumber(
      const std::string& longOpt,
      char shortOpt,
      const std::string& valuePlaceholder,
      const std::string& helpText,
      std::function<void(long int)> callback = nullptr);

  // defaulted number flag
  CLI& defineNumber(
      const std::string& longOpt,
      char shortOpt,
      const std::string& valuePlaceholder,
      const std::string& helpText,
      long int defaultValue,
      std::function<void(long int)> callback = nullptr);

  // required floating-number flag
  CLI& defineFloat(
      const std::string& longOpt,
      char shortOpt,
      const std::string& valuePlaceholder,
      const std::string& helpText,
      std::function<void(float)> callback = nullptr);

  // defaulted floating-number flag
  CLI& defineFloat(
      const std::string& longOpt,
      char shortOpt,
      const std::string& valuePlaceholder,
      const std::string& helpText,
      float defaultValue,
      std::function<void(float)> callback = nullptr);

  // required IP-address flag
  CLI& defineIPAddress(
      const std::string& longOpt,
      char shortOpt,
      const std::string& valuePlaceholder,
      const std::string& helpText,
      std::function<void(const IPAddress&)> callback = nullptr);

  // defaulted IP-address flag
  CLI& defineIPAddress(
      const std::string& longOpt,
      char shortOpt,
      const std::string& valuePlaceholder,
      const std::string& helpText,
      const IPAddress& defaultValue,
      std::function<void(const IPAddress&)> callback = nullptr);

  // defaulted bool flag (always defaults to false)
  CLI& defineBool(
      const std::string& longOpt,
      char shortOpt,
      const std::string& helpText,
      std::function<void(bool)> callback = nullptr);

  // whether or not to allow unnamed raw parameter values
  CLI& enableParameters(
      const std::string& valuePlaceholder,
      const std::string& helpText);

  const FlagDef* find(const std::string& longOption) const;
  const FlagDef* find(char shortOption) const;

  const FlagDef* require(const std::string& longOption) const;
  const FlagDef* require(char shortOption) const;

  Flags evaluate(int argc, const char* argv[]) const;
  Flags evaluate(const std::vector<std::string>& args) const;

  std::string helpText(size_t width = 78, size_t helpTextOffset = 30) const;

 private:
  CLI& define(
      const std::string& longOpt,
      char shortOpt,
      bool required,
      FlagType type,
      const std::string& helpText,
      const std::string& valuePlaceholder,
      const std::string& defaultValue,
      std::function<void(const std::string&)> callback);

 private:
  std::list<FlagDef> flagDefs_;

  // non-option parameters
  bool parametersEnabled_;
  std::string parametersPlaceholder_;
  std::string parametersHelpText_;
};

class CORTEX_API CLI::TypeMismatchError : public RuntimeError {
 public:
  explicit TypeMismatchError(const RuntimeError& v) : RuntimeError(v) {}
  TypeMismatchError()
    : RuntimeError((int) Status::CliTypeMismatchError, StatusCategory::get()) {}
};

class CORTEX_API CLI::UnknownOptionError : public RuntimeError {
 public:
  explicit UnknownOptionError(const RuntimeError& v) : RuntimeError(v) {}
  UnknownOptionError()
    : RuntimeError((int) Status::CliUnknownOptionError, StatusCategory::get()) {}
};

class CORTEX_API CLI::MissingOptionError : public RuntimeError {
 public:
  explicit MissingOptionError(const RuntimeError& v) : RuntimeError(v) {}
  MissingOptionError()
    : RuntimeError((int) Status::CliMissingOptionError, StatusCategory::get()) {}
};

class CORTEX_API CLI::MissingOptionValueError : public RuntimeError {
 public:
  explicit MissingOptionValueError(const RuntimeError& v) : RuntimeError(v) {}
  MissingOptionValueError()
    : RuntimeError((int) Status::CliMissingOptionValueError, StatusCategory::get()) {}
};


class CORTEX_API FlagBuilder {
 public:
  FlagBuilder(CLI& cli) : cli_(cli) {}

  CLI& cli() const { return cli_; }

 private:
  CLI& cli_;
};

struct CORTEX_API CLI::FlagDef {
  FlagType type;
  std::string longOption;
  char shortOption;
  bool required;
  std::string valuePlaceholder;
  std::string helpText;
  std::string defaultValue;
  std::function<void(const std::string&)> callback;

  std::string makeHelpText(size_t width, size_t helpTextOffset) const;
};

}  // namespace cortex
