/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Laura Schlimmer <laura@eventql.io>
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
#include <eventql/sql/qtree/nodes/set.h>

namespace csql {

SetNode::SetNode(
        const std::string& variable,
        const std::string value) :
        variable_(variable),
        value_(value) {}

SetNode::SetNode(const SetNode& node) {}

const std::string& SetNode::getVariable() const {
  return variable_;
}

const std::string& SetNode::getValue() const {
  return value_;
}

RefPtr<QueryTreeNode> SetNode::deepCopy() const {
  return new SetNode(*this);
}

std::string SetNode::toString() const {
  return "(set)";
}

} // namespace csql

