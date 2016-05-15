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

namespace csql {

template <class T>
void QueryTreeCoder::registerType(uint64_t wire_type_id) {
  QueryTreeCoderType type;
  type.type_id = &typeid(T);
  type.wire_type_id = wire_type_id;

  type.encode_fn = [] (QueryTreeCoder* coder, RefPtr<QueryTreeNode> self, stx::OutputStream* os) {
    T::encode(coder, *dynamic_cast<T*>(self.get()), os);
  };

  type.decode_fn = [] (QueryTreeCoder* coder, stx::InputStream* is) -> RefPtr<QueryTreeNode> {
    return T::decode(coder, is);
  };

  registerType(type);
}

} // namespace csql
