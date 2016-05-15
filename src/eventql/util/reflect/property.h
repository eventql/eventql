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
#ifndef _STX_REFLECT_PROPERTY_H
#define _STX_REFLECT_PROPERTY_H
#include <vector>
#include <string>
#include <functional>
#include "eventql/util/reflect/indexsequence.h"

namespace stx {
namespace reflect {

template <typename ClassType, typename TargetType>
class PropertyReader {
public:
  PropertyReader(TargetType* target);

  template <typename PropertyType>
  void prop(
      PropertyType prop,
      uint32_t id,
      const std::string& prop_name,
      bool optional);

  const ClassType& instance() const;

protected:
  ClassType instance_;
  TargetType* target_;
};

template <typename ClassType, typename TargetType>
class PropertyWriter {
public:
  PropertyWriter(const ClassType& instance, TargetType* target);

  template <typename PropertyType>
  void prop(
      PropertyType prop,
      uint32_t id,
      const std::string& prop_name,
      bool optional);

protected:
  const ClassType& instance_;
  TargetType* target_;
};

template <typename TargetType>
class PropertyProxy {
public:
  PropertyProxy(TargetType* target);

  template <typename PropertyType>
  void prop(
      PropertyType prop,
      uint32_t id,
      const std::string& prop_name,
      bool optional);

protected:
  TargetType* target_;
};

}
}

#include "property_impl.h"
#endif
