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
#include <eventql/util/protobuf/MessageBuilder.h>
#include <eventql/util/inspect.h>
#include <eventql/util/exception.h>

namespace stx {
namespace msg {

void MessageBuilder::setUInt32(const String path, uint32_t value) {
  FieldValue val;
  val.type = FieldType::UINT32;
  val.value = Buffer(&value, sizeof(value));
  fields_[path] = val;
}

void MessageBuilder::setBool(const String path, bool value) {
  uint8_t v = value ? 1 : 0;
  FieldValue val;
  val.type = FieldType::BOOLEAN;
  val.value = Buffer(&v, sizeof(v));
  fields_[path] = val;
}

void MessageBuilder::setString(const String path, const String& value) {
  FieldValue val;
  val.type = FieldType::STRING;
  val.value = Buffer(value.data(), value.size());
  fields_[path] = val;
}

void MessageBuilder::encode(const MessageSchema& schema, Buffer* buf) {
  util::BinaryMessageWriter writer;

  for (const auto& f : schema.fields) {
    encodeField("", f, &writer);
  }

  buf->append(writer.data(), writer.size()); // FIXPAUL!!
}

void MessageBuilder::encodeField(
    const String& prefix,
    const MessageSchemaField& field,
    util::BinaryMessageWriter* buf) {
  if (field.repeated) {
    auto n = countRepetitions(prefix + field.name);

    for (int i = 0; i < n; ++i) {
      encodeSingleField(
          StringUtil::format("$0$1[$2]", prefix, field.name, i),
          field,
          buf);
    }
  } else {
    encodeSingleField(prefix + field.name, field, buf);
  }
}

void MessageBuilder::encodeSingleField(
    const String& path,
    const MessageSchemaField& field,
    util::BinaryMessageWriter* buf) {
  buf->appendUInt32(field.id);

  if (field.type == FieldType::OBJECT) {
    for (const auto& f : field.fields) {
      encodeField(path + ".", f, buf);
    }

    return;
  }

  auto fdata = fields_.find(path);
  if (fdata == fields_.end()) {
    if (field.optional) {
      return;
    } else {
      RAISEF(kIllegalArgumentError, "missing value for field '$0'", path);
    }
  }

  if (fdata->second.type != field.type) {
    RAISEF(kIllegalArgumentError, "field '$0' has invalid type", path);
  }

  const auto& fval = fdata->second.value;
  switch (field.type) {

    case FieldType::STRING:
      buf->appendUInt32(fval.size());
      buf->append(fval.data(), fval.size());
      break;

    case FieldType::BOOLEAN:
      buf->appendUInt32(0);
      break;

    case FieldType::UINT32:
      buf->appendUInt32(123);
      break;

    case FieldType::OBJECT:
      break;

  }
}

size_t MessageBuilder::countRepetitions(const String& prefix) const {
  auto rprefix = prefix + "[";

  Set<size_t> reps;

  for (const auto& f : fields_) {
    if (!StringUtil::beginsWith(f.first, rprefix)) {
      continue;
    }

    auto s = f.first.substr(rprefix.size());
    s.erase(s.begin() + StringUtil::find(s, ']'), s.end());
    reps.emplace(std::stoul(s));
  }

  for (size_t i = 0; i < reps.size(); ++i) {
    if (reps.count(i) == 0) {
      RAISEF(kIllegalArgumentError, "missing index $0 for '$1'", i, prefix);
    }
  }

  return reps.size();
}

bool MessageBuilder::isSet(const String& path) const {
  return fields_.count(path) > 0;
}

} // namespace msg
} // namespace stx

