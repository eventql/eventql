/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *   - Laura Schlimmer <laura@zscale.io>
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
#pragma once
#include <eventql/eventql.h>
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/util/option.h>
#include <eventql/util/status.h>

namespace eventql {

class ProcessConfig : public RefCounted {
friend class ProcessConfigBuilder;
public:

  Option<String> getString(const String& key) const;
  Option<String> getString(const String& section, const String& key) const;
  Option<int64_t> getInt(const String& key) const;
  Option<int64_t> getInt(const String& section, const String& key) const;
  bool getBool(const String& key) const;
  bool hasProperty(const String& key) const;

protected:
  ProcessConfig(HashMap<String, String> properties);
  HashMap<String, String> properties_;
};

class ProcessConfigBuilder {
public:

  Status loadFile(const String& file);
  Status loadDefaultConfigFile();

  void setProperty(const String& key, const String& value);
  void setProperty(
      const String& section,
      const String& key,
      const String& value);

  RefPtr<ProcessConfig> getConfig();

protected:
  HashMap<String, String> properties_;
};


} // namespace eventql



