/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#ifndef _STX_CLI_FLAGPARSER_H
#define _STX_CLI_FLAGPARSER_H
#include <string>
#include <vector>
#include <eventql/util/exception.h>
#include <eventql/util/io/outputstream.h>

namespace util {
namespace cli {

class FlagParser {
public:
  enum kFlagType {
    T_SWITCH,
    T_STRING,
    T_FLOAT,
    T_INTEGER
  };

  FlagParser();

  /**
   * Define a flag
   */
  void defineFlag(
      const char* longopt,
      kFlagType type,
      bool required,
      const char* shortopt = NULL,
      const char* default_value = NULL,
      const char* description = NULL,
      const char* placeholder = NULL);

  /**
   * Returns true if the flag is set and false otherwise
   *
   * @param longopt the longopt of the flag
   */
  bool isSet(const char* longopt) const;

  /**
   * Returns the string value of the flag or throws an exception if the value
   * is invalid.
   *
   * @param longopt the longopt of the flag
   */
  std::string getString(const char* longopt) const;

  /**
   * Returns the string values of the flag or throws an exception if the value
   * is invalid.
   *
   * @param longopt the longopt of the flag
   */
  Vector<std::string> getStrings(const char* longopt) const;

  /**
   * Returns the integer value of the flag or throws an exception if the value
   * is invalid.
   *
   * @param longopt the longopt of the flag
   */
  int64_t getInt(const char* longopt) const;

  /**
   * Parse an argv array. This may throw an exception.
   */
  void parseArgv(int argc, const char** argv);

  /**
   * Parse an argv array. This may throw an exception.
   */
  void parseArgv(const std::vector<std::string>& argv);

  /**
   * Get the argv array with all flag args removed
   */
  const std::vector<std::string>& getArgv() const;

  void printUsage(util::OutputStream* target) const;

  void ignoreUnknownFlags();

protected:
  struct FlagState {
    kFlagType type;
    bool required;
    const char* longopt;
    const char* shortopt;
    const char* default_value;
    const char* description;
    const char* placeholder;
    std::vector<std::string> values;
  };

  std::vector<std::string> argv_;
  std::vector<FlagState> flags_;
  bool ignore_unknown_;
};

}
}
#endif
