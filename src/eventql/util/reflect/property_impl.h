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
namespace reflect {

template <typename ClassType, typename TargetType>
PropertyReader<ClassType, TargetType>::PropertyReader(
    TargetType* target) :
    target_(target) {}

template <typename ClassType, typename TargetType>
const ClassType& PropertyReader<ClassType, TargetType>::instance() const {
  return instance_;
}

template <typename ClassType, typename TargetType>
template <typename PropertyType>
void PropertyReader<ClassType, TargetType>::prop(
    PropertyType prop,
    uint32_t id,
    const std::string& prop_name,
    bool optional) {
  if (optional) {
    auto opt = target_->template getOptionalProperty<
        typename std::decay<decltype(instance_.*prop)>::type>(
            id,
            prop_name);

    if (!opt.isEmpty()) {
      instance_.*prop = opt.get();
    }
  } else {
    instance_.*prop = target_->template getProperty<
        typename std::decay<decltype(instance_.*prop)>::type>(
            id,
            prop_name);
  }
}

template <typename ClassType, typename TargetType>
PropertyWriter<ClassType, TargetType>::PropertyWriter(
    const ClassType& instance,
    TargetType* target) :
    instance_(instance),
    target_(target) {}

template <typename ClassType, typename TargetType>
template <typename PropertyType>
void PropertyWriter<ClassType, TargetType>::prop(
    PropertyType prop,
    uint32_t id,
    const std::string& prop_name,
    bool optional) {
  target_->putProperty(id, prop_name, instance_.*prop);
}

template <class ClassType>
template <class TargetType>
void MetaClass<ClassType>::serialize(
    const ClassType& instance,
    TargetType* target) {
  PropertyWriter<ClassType, TargetType> writer(instance, target);
  reflect(&writer);
}

template <class ClassType>
template <class TargetType>
ClassType MetaClass<ClassType>::unserialize(
    TargetType* target) {
  PropertyReader<ClassType, TargetType> reader(target);
  reflect(&reader);
  return reader.instance();
}


}
