/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#ifndef _STX_REFLECT_METACLASS_H
#define _STX_REFLECT_METACLASS_H
#include <unordered_map>

namespace reflect {

template <class ClassType>
class MetaClass {
public:
  template <class TargetType>
  static void reflectMethods(TargetType* target);

  template <class TargetType>
  static void reflectProperties(TargetType* target);

  template <class TargetType>
  static void reflect(TargetType* target);

  template <class TargetType>
  static void serialize(const ClassType& instance, TargetType* target);

  template <class TargetType>
  static ClassType unserialize(TargetType* target);

};

template <typename TargetType>
struct DummyTarget {};

template<typename T, typename X = void>
struct is_reflected : std::false_type {};

template<typename T>
struct is_reflected<
    T,
    decltype(T::reflect((reflect::DummyTarget<T>*) nullptr))> :
        std::true_type {};

template <class ClassType>
template <class TargetType>
void MetaClass<ClassType>::reflect(TargetType* target) {
  ClassType::reflect(target);
}

}
#endif
