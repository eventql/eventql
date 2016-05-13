/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include "eventql/util/stdtypes.h"
#include "eventql/util/autoref.h"
#include "eventql/util/protobuf/MessageObject.h"
#include "eventql/CustomerConfigProtos.pb.h"

using namespace stx;

namespace zbase {

struct CustomerConfigRef : public RefCounted {
  CustomerConfigRef(CustomerConfig _config) : config(_config) {}
  CustomerConfig config;
};

CustomerConfig createCustomerConfig(const String& customer);

void eventDefinitonRemoveField(EventDefinition* def, const String& field);

void eventDefinitonAddField(
    EventDefinition* def,
    const String& field,
    uint32_t id,
    msg::FieldType type,
    bool repeated,
    bool optional);

} // namespace zbase

