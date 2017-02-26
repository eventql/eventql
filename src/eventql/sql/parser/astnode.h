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

#ifndef _FNORDMETRIC_QUERY_ASTNODE_H
#define _FNORDMETRIC_QUERY_ASTNODE_H
#include <stdlib.h>
#include <string>
#include <vector>

namespace csql {
class Token;

class ASTNode {
  friend class QueryTest;
public:
  enum kASTNodeType {
    T_ROOT,

    T_LITERAL,
    T_METHOD_CALL,
    T_METHOD_CALL_WITHIN_RECORD,
    T_RESOLVED_CALL,
    T_COLUMN_NAME,
    T_COLUMN_ALIAS,
    T_COLUMN_INDEX,
    T_RESOLVED_COLUMN,
    T_TABLE_NAME,
    T_TABLE_ALIAS,
    T_DERIVED_COLUMN,
    T_PROPERTY,
    T_PROPERTY_VALUE,
    T_VOID,

    T_SELECT,
    T_SELECT_DEEP,
    T_SELECT_LIST,
    T_ALL,
    T_FROM,
    T_WHERE,
    T_GROUP_BY,
    T_ORDER_BY,
    T_SORT_SPEC,
    T_HAVING,
    T_LIMIT,
    T_OFFSET,

    T_JOIN_CONDITION,
    T_JOIN_COLUMNLIST,
    T_INNER_JOIN,
    T_LEFT_JOIN,
    T_RIGHT_JOIN,
    T_NATURAL_INNER_JOIN,
    T_NATURAL_LEFT_JOIN,
    T_NATURAL_RIGHT_JOIN,

    T_IF_EXPR,
    T_EQ_EXPR,
    T_NEQ_EXPR,
    T_LT_EXPR,
    T_LTE_EXPR,
    T_GT_EXPR,
    T_GTE_EXPR,
    T_AND_EXPR,
    T_OR_EXPR,
    T_NEGATE_EXPR,
    T_ADD_EXPR,
    T_SUB_EXPR,
    T_MUL_EXPR,
    T_DIV_EXPR,
    T_MOD_EXPR,
    T_POW_EXPR,
    T_REGEX_EXPR,
    T_LIKE_EXPR,

    T_SHOW_TABLES,
    T_DESCRIBE_TABLE,
    T_DESCRIBE_PARTITIONS,
    T_CLUSTER_SHOW_SERVERS,
    T_EXPLAIN_QUERY,
    T_CREATE_TABLE,
    T_COLUMN_LIST,
    T_PRIMARY_KEY,
    T_PARTITION_KEY,
    T_RECORD,
    T_REPEATED,
    T_NOT_NULL,
    T_COLUMN,
    T_COLUMN_TYPE,
    T_TABLE_PROPERTY_LIST,
    T_TABLE_PROPERTY,
    T_TABLE_PROPERTY_KEY,
    T_TABLE_PROPERTY_VALUE,
    T_DROP_TABLE,
    T_DATABASE_NAME,
    T_CREATE_DATABASE,
    T_USE_DATABASE,
    T_INSERT_INTO,
    T_VALUE_LIST,
    T_JSON_STRING,
    T_ALTER_TABLE,

    T_DRAW,
    T_IMPORT,
    T_AXIS,
    T_AXIS_POSITION,
    T_AXIS_LABELS,
    T_DOMAIN,
    T_DOMAIN_SCALE,
    T_GRID,
    T_LEGEND,
    T_GROUP_OVER_TIMEWINDOW
  };

  ASTNode(kASTNodeType type);
  bool operator==(kASTNodeType type) const;
  bool operator==(const ASTNode& other) const;

  // compare asts recursively. returns true if both ast trees are equal
  bool compare(const ASTNode* other);

  ASTNode* appendChild(ASTNode::kASTNodeType type);
  void appendChild(ASTNode* node);
  void appendChild(ASTNode* node, size_t index);
  void removeChildByIndex(size_t index);
  void removeChildrenByType(kASTNodeType type);
  void removeChild(ASTNode* node);
  void clearChildren();
  const std::vector<ASTNode*>& getChildren() const;
  void setToken(const Token* token);
  void clearToken();
  const Token* getToken() const;
  kASTNodeType getType() const;
  void setType(kASTNodeType type);
  uint64_t getID() const;
  void setID(uint64_t id);

  ASTNode* deepCopy() const;

  void debugPrint(int indent = 2) const;

protected:
  kASTNodeType type_;
  const Token* token_;
  int64_t id_;
  std::vector<ASTNode*> children_;
};


}
#endif
