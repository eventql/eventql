/**
 * This file is part of the "sensord" project
 *   Copyright (c) 2015 Paul Asmuth
 *   Copyright (c) 2015 Finn Zirngibl
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <fnord/protobuf/msg.h>

namespace sensord {

template <typename ProtoType>
ProtoSensor<ProtoType>::ProtoSensor() :
    schema_(msg::MessageSchema::fromProtobuf(ProtoType::descriptor())) {}

template <typename ProtoType>
BufferRef ProtoSensor<ProtoType>::fetchData() const {
  ProtoType proto;
  fetchData(&proto);
  return msg::encode(proto);
}

template <typename ProtoType>
RefPtr<msg::MessageSchema> ProtoSensor<ProtoType>::schema() const {
  return schema_;
}

};

