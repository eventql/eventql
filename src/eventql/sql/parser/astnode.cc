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
#include <stdlib.h>
#include <assert.h>
#include "astnode.h"
#include "token.h"
#include <eventql/util/inspect.h>

namespace csql {

ASTNode::ASTNode(kASTNodeType type) :
    type_(type),
    token_(nullptr),
    id_(-1) {}

bool ASTNode::operator==(kASTNodeType type) const {
  return type_ == type;
}

bool ASTNode::operator==(const ASTNode& other) const {
  if (type_ != other.type_) {
    return false;
  }

  if (token_ == nullptr) {
    return other.token_ == nullptr;
  } else {
    return *token_ == *other.token_;
  }
}

bool ASTNode::compare(const ASTNode* other) {
  if (type_ != other->type_) {
    return false;
  }

  if (!((token_ == nullptr && other->token_ == nullptr) ||
      (token_ && other->token_ && *token_ == *other->token_))) {
    return false;
  }

  if (children_.size() != other->children_.size()) {
    return false;
  }

  for (size_t i = 0; i < children_.size(); ++i) {
    if (!children_[i]->compare(other->children_[i])) {
      return false;
    }
  }

  return true;
}

ASTNode* ASTNode::appendChild(ASTNode::kASTNodeType type) {
  auto child = new ASTNode(type);
  children_.push_back(child);
  return child;
}

void ASTNode::clearChildren() {
  children_.clear();
}

void ASTNode::removeChildByIndex(size_t index) {
  children_.erase(children_.begin() + index);
}

void ASTNode::removeChild(ASTNode* node) {
  for (auto child = children_.begin(); child < children_.end(); ++child) {
    if (*child == node) {
      children_.erase(child);
      return;
    }
  }
}

void ASTNode::removeChildrenByType(kASTNodeType type) {
  for (auto child = children_.begin(); child < children_.end(); ++child) {
    if ((*child)->getType() == type) {
      children_.erase(child);
    }
  }
}

void ASTNode::appendChild(ASTNode* node) {
  children_.push_back(node);
}

void ASTNode::appendChild(ASTNode* node, size_t index) {
  children_.insert(children_.begin() + index, node);
}

const std::vector<ASTNode*>& ASTNode::getChildren() const {
  return children_;
}

void ASTNode::setToken(const Token* token) {
  token_ = token;
}

void ASTNode::clearToken() {
  token_ = nullptr;
}

const Token* ASTNode::getToken() const {
  return token_;
}

ASTNode::kASTNodeType ASTNode::getType() const {
  return type_;
}

void ASTNode::setType(kASTNodeType type) {
  type_ = type;
}

uint64_t ASTNode::getID() const {
  return id_;
}

void ASTNode::setID(uint64_t id) {
  id_ = id;
}

ASTNode* ASTNode::deepCopy() const {
  auto copy = new ASTNode(type_);

  if (token_ != nullptr) {
    copy->setToken(new Token(*token_));
  }

  for (const auto child : children_) {
    if (child == nullptr) {
      continue;
    }

    copy->appendChild(child->deepCopy());
  }

  return copy;
}

void ASTNode::debugPrint(int indent /* = 0 */) const {
  for (int i = 0; i < indent - 2; i++) {
    printf("  ");
  }

  switch (type_) {
    case T_ROOT:
      printf("- T_ROOT");
      break;
    case T_LITERAL:
      printf("- T_LITERAL");
      break;
    case T_METHOD_CALL:
      printf("- T_METHOD_CALL");
      break;
    case T_METHOD_CALL_WITHIN_RECORD:
      printf("- T_METHOD_CALL_WITHIN_RECORD");
      break;
    case T_RESOLVED_CALL:
      printf("- T_RESOLVED_CALL");
      break;
    case T_COLUMN_NAME:
      printf("- T_COLUMN_NAME");
      break;
    case T_COLUMN_ALIAS:
      printf("- T_COLUMN_ALIAS");
      break;
    case T_COLUMN_INDEX:
      printf("- T_COLUMN_INDEX");
      break;
    case T_RESOLVED_COLUMN:
      printf("- T_RESOLVED_COLUMN");
      break;
    case T_TABLE_NAME:
      printf("- T_TABLE_NAME");
      break;
    case T_TABLE_ALIAS:
      printf("- T_TABLE_ALIAS");
      break;
    case T_DERIVED_COLUMN:
      printf("- T_DERIVED_COLUMN");
      break;
    case T_PROPERTY:
      printf("- T_PROPERTY");
      break;
    case T_PROPERTY_VALUE:
      printf("- T_PROPERTY_VALUE");
      break;
    case T_VOID:
      printf("- T_VOID");
      break;
    case T_SELECT:
      printf("- T_SELECT");
      break;
    case T_SELECT_DEEP:
      printf("- T_SELECT_DEEP");
      break;
    case T_SELECT_LIST:
      printf("- T_SELECT_LIST");
      break;
    case T_ALL:
      printf("- T_ALL");
      break;
    case T_FROM:
      printf("- T_FROM");
      break;
    case T_WHERE:
      printf("- T_WHERE");
      break;
    case T_GROUP_BY:
      printf("- T_GROUP_BY");
      break;
    case T_ORDER_BY:
      printf("- T_ORDER_BY");
      break;
    case T_SORT_SPEC:
      printf("- T_SORT_SPEC");
      break;
    case T_HAVING:
      printf("- T_HAVING");
      break;
    case T_LIMIT:
      printf("- T_LIMIT");
      break;
    case T_OFFSET:
      printf("- T_OFFSET");
      break;
    case T_JOIN_CONDITION:
      printf("- T_JOIN_CONDITION");
      break;
    case T_JOIN_COLUMNLIST:
      printf("- T_JOIN_COLUMNLIST");
      break;
    case T_INNER_JOIN:
      printf("- T_INNER_JOIN");
      break;
    case T_LEFT_JOIN:
      printf("- T_LEFT_JOIN");
      break;
    case T_RIGHT_JOIN:
      printf("- T_RIGHT_JOIN");
      break;
    case T_NATURAL_INNER_JOIN:
      printf("- T_NATURAL_INNER_JOIN");
      break;
    case T_NATURAL_LEFT_JOIN:
      printf("- T_NATURAL_LEFT_JOIN");
      break;
    case T_NATURAL_RIGHT_JOIN:
      printf("- T_NATURAL_RIGHT_JOIN");
      break;
    case T_IF_EXPR:
      printf("- T_IF_EXPR");
      break;
    case T_EQ_EXPR:
      printf("- T_EQ_EXPR");
      break;
    case T_NEQ_EXPR:
      printf("- T_NEQ_EXPR");
      break;
    case T_LT_EXPR:
      printf("- T_LT_EXPR");
      break;
    case T_LTE_EXPR:
      printf("- T_LTE_EXPR");
      break;
    case T_GT_EXPR:
      printf("- T_GT_EXPR");
      break;
    case T_GTE_EXPR:
      printf("- T_GTE_EXPR");
      break;
    case T_AND_EXPR:
      printf("- T_AND_EXPR");
      break;
    case T_OR_EXPR:
      printf("- T_OR_EXPR");
      break;
    case T_NEGATE_EXPR:
      printf("- T_NEGATE_EXPR");
      break;
    case T_ADD_EXPR:
      printf("- T_ADD_EXPR");
      break;
    case T_SUB_EXPR:
      printf("- T_SUB_EXPR");
      break;
    case T_MUL_EXPR:
      printf("- T_MUL_EXPR");
      break;
    case T_DIV_EXPR:
      printf("- T_DIV_EXPR");
      break;
    case T_MOD_EXPR:
      printf("- T_MOD_EXPR");
      break;
    case T_POW_EXPR:
      printf("- T_POW_EXPR");
      break;
    case T_REGEX_EXPR:
      printf("- T_REGEX_EXPR");
      break;
    case T_LIKE_EXPR:
      printf("- T_LIKE_EXPR");
      break;
    case T_SHOW_TABLES:
      printf("- T_SHOW_TABLES");
      break;
    case T_DESCRIBE_TABLE:
      printf("- T_DESCRIBE_TABLE");
      break;
    case T_DESCRIBE_PARTITIONS:
      printf("- T_DESCRIBE_PARTITIONS");
      break;
    case T_DESCRIBE_SERVERS:
      printf("- T_DESCRIBE_SERVERS");
      break;
    case T_EXPLAIN_QUERY:
      printf("- T_EXPLAIN_QUERY");
      break;
    case T_CREATE_TABLE:
      printf("- T_CREATE_TABLE");
      break;
    case T_COLUMN_LIST:
      printf("- T_COLUMN_LIST");
      break;
    case T_PRIMARY_KEY:
      printf("- T_PRIMARY_KEY");
      break;
    case T_RECORD:
      printf("- T_RECORD");
      break;
    case T_REPEATED:
      printf("- T_REPEATED");
      break;
    case T_NOT_NULL:
      printf("- T_NOT_NULL");
      break;
    case T_COLUMN:
      printf("- T_COLUMN");
      break;
    case T_COLUMN_TYPE:
      printf("- T_COLUMN_TYPE");
      break;
    case T_TABLE_PROPERTY_LIST:
      printf("- T_TABLE_PROPERTY_LIST");
      break;
    case T_TABLE_PROPERTY:
      printf("- T_TABLE_PROPERTY");
      break;
    case T_TABLE_PROPERTY_KEY:
      printf("- T_TABLE_PROPERTY_KEY");
      break;
    case T_TABLE_PROPERTY_VALUE:
      printf("- T_TABLE_PROPERTY_VALUE");
      break;
    case T_DATABASE_NAME:
      printf("- T_DATABASE_NAME");
      break;
    case T_CREATE_DATABASE:
      printf("- T_CREATE_DATABASE");
      break;
    case T_USE_DATABASE:
      printf("- T_USE_DATABASE");
      break;
    case T_DROP_TABLE:
      printf("- T_DROP_TABLE");
      break;
    case T_INSERT_INTO:
      printf("- T_INSERT_INTO");
      break;
    case T_VALUE_LIST:
      printf("- T_VALUE_LIST");
      break;
    case T_JSON_STRING:
      printf("- T_JSON_STRING");
      break;
    case T_ALTER_TABLE:
      printf("- T_ALTER_TABLE");
      break;
    case T_DRAW:
      printf("- T_DRAW");
      break;
    case T_IMPORT:
      printf("- T_IMPORT");
      break;
    case T_AXIS:
      printf("- T_AXIS");
      break;
    case T_AXIS_POSITION:
      printf("- T_AXIS_POSITION");
      break;
    case T_AXIS_LABELS:
      printf("- T_AXIS_LABELS");
      break;
    case T_DOMAIN:
      printf("- T_DOMAIN");
      break;
    case T_DOMAIN_SCALE:
      printf("- T_DOMAIN_SCALE");
      break;
    case T_GRID:
      printf("- T_GRID");
      break;
    case T_LEGEND:
      printf("- T_LEGEND");
      break;
    case T_GROUP_OVER_TIMEWINDOW:
      printf("- T_GROUP_OVER_TIMEWINDOW");
      break;
    default:
      printf("- <unknown ASTNode>");
      break;
  }

  if (token_ != nullptr) {
    printf(
        " [%s] (%s)",
        Token::getTypeName(token_->getType()),
        token_->getString().c_str());
  }

  if (id_ != -1) {
    printf(" <%lli>", id_);
  }

  printf("\n");

  for (const auto child : children_) {
    if (child == nullptr) {

      for (int i = 0; i < indent - 1; i++) {
        printf("  ");
      }

      printf("- <nullptr>\n");
    } else {
      child->debugPrint(indent + 1);
    }
  }
}

} // namespace csql

template <>
std::string inspect<
    csql::ASTNode::kASTNodeType>(
    const csql::ASTNode::kASTNodeType& value) {
  return "<ASTNode>";
}

template <>
std::string inspect<
    csql::ASTNode>(
    const csql::ASTNode& value) {
  return "<ASTNode>";
}

