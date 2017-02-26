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
#include <eventql/sql/parser/token.h>
#include <eventql/util/exception.h>
#include <eventql/util/inspect.h>
#include <string.h>

namespace csql {

Token::Token(kTokenType token_type) :
    type_(token_type) {}

Token::Token(
    kTokenType token_type,
    const char* data,
    size_t len) :
    type_(token_type) {
  if (data != nullptr) {
    str_ = std::string(data, len);
  }
}

Token::Token(
    kTokenType token_type,
    const std::string& str) :
    type_(token_type),
    str_(str) {}

Token::Token(const Token& copy) :
    type_(copy.type_),
    str_(copy.str_) {}

void Token::debugPrint() const {
  if (str_.size() == 0) {
    printf("%s\n", getTypeName(type_));
  } else {
    printf("%s(%s)\n", getTypeName(type_), str_.c_str());
  }
}

const char* Token::getTypeName(kTokenType type) {
  switch (type) {
    case T_SELECT: return "T_SELECT";
    case T_FROM: return "T_FROM";
    case T_WHERE: return "T_WHERE";
    case T_GROUP: return "T_GROUP";
    case T_ORDER: return "T_ORDER";
    case T_BY: return "T_BY";
    case T_HAVING: return "T_HAVING";
    case T_LIMIT: return "T_LIMIT";
    case T_OFFSET: return "T_OFFSET";
    case T_ASC: return "T_ASC";
    case T_DESC: return "T_DESC";
    case T_COMMA: return "T_COMMA";
    case T_DOT: return "T_DOT";
    case T_IDENTIFIER: return "T_IDENTIFIER";
    case T_STRING: return "T_STRING";
    case T_NUMERIC: return "T_NUMERIC";
    case T_SEMICOLON: return "T_SEMICOLON";
    case T_LPAREN: return "T_LPAREN";
    case T_RPAREN: return "T_RPAREN";
    case T_AND: return "T_AND";
    case T_OR: return "T_OR";
    case T_EQUAL: return "T_EQUAL";
    case T_NEQUAL: return "T_NEQUAL";
    case T_PLUS: return "T_PLUS";
    case T_MINUS: return "T_MINUS";
    case T_ASTERISK: return "T_ASTERISK";
    case T_SLASH: return "T_SLASH";
    case T_AS: return "T_AS";
    case T_NOT: return "T_NOT";
    case T_TRUE: return "T_TRUE";
    case T_FALSE: return "T_FALSE";
    case T_BANG: return "T_BANG";
    case T_CIRCUMFLEX: return "T_CIRCUMFLEX";
    case T_TILDE: return "T_TILDE";
    case T_PERCENT: return "T_PERCENT";
    case T_DIV: return "T_DIV";
    case T_MOD: return "T_MOD";
    case T_AMPERSAND: return "T_AMPERSAND";
    case T_PIPE: return "T_PIPE";
    case T_LSHIFT: return "T_LSHIFT";
    case T_RSHIFT: return "T_RSHIFT";
    case T_LT: return "T_LT";
    case T_LTE: return "T_LTE";
    case T_GT: return "T_GT";
    case T_GTE: return "T_GTE";
    case T_BEGIN: return "T_BEGIN";
    case T_WITHIN: return "T_WITHIN";
    case T_RECORD: return "T_RECORD";
    case T_CREATE: return "T_CREATE";
    case T_WITH: return "T_WITH";
    case T_IMPORT: return "T_IMPORT";
    case T_TABLE: return "T_TABLE";
    case T_TABLES: return "T_TABLES";
    case T_DATABASE: return "T_DATABASE";
    case T_USE: return "T_USE";
    case T_ON: return "T_ON";
    case T_OFF: return "T_OFF";
    case T_ALTER: return "T_ALTER";
    case T_ADD: return "T_ADD";
    case T_DROP: return "T_DROP";
    case T_SET: return "T_SET";
    case T_PROPERTY: return "T_PROPERTY";
    case T_PRIMARY: return "T_PRIMARY";
    case T_PARTITION: return "T_PARTITION";
    case T_KEY: return "T_KEY";
    case T_COLUMN: return "T_COLUMN";
    case T_SHOW: return "T_SHOW";
    case T_DESCRIBE: return "T_DESCRIBE";
    case T_EXPLAIN: return "T_EXPLAIN";
    case T_PARTITIONS: return "T_PARTITIONS";
    case T_CLUSTER: return "T_CLUSTER";
    case T_SERVERS: return "T_SERVERS";
    case T_EOF: return "T_EOF";
    case T_DRAW: return "T_DRAW";
    case T_LINECHART: return "T_LINECHART";
    case T_AREACHART: return "T_AREACHART";
    case T_BARCHART: return "T_BARCHART";
    case T_POINTCHART: return "T_POINTCHART";
    case T_HEATMAP: return "T_HEATMAP";
    case T_HISTOGRAM: return "T_HISTOGRAM";
    case T_AXIS: return "T_AXIS";
    case T_TOP: return "T_TOP";
    case T_RIGHT: return "T_RIGHT";
    case T_BOTTOM: return "T_BOTTOM";
    case T_LEFT: return "T_LEFT";
    case T_ORIENTATION: return "T_ORIENTATION";
    case T_HORIZONTAL: return "T_HORIZONTAL";
    case T_VERTICAL: return "T_VERTICAL";
    case T_STACKED: return "T_STACKED";
    case T_XDOMAIN: return "T_XDOMAIN";
    case T_YDOMAIN: return "T_YDOMAIN";
    case T_ZDOMAIN: return "T_ZDOMAIN";
    case T_XGRID: return "T_XGRID";
    case T_YGRID: return "T_YGRID";
    case T_LOGARITHMIC: return "T_LOGARITHMIC";
    case T_INVERT: return "T_INVERT";
    case T_TITLE: return "T_TITLE";
    case T_SUBTITLE: return "T_SUBTITLE";
    case T_GRID: return "T_GRID";
    case T_LABELS: return "T_LABELS";
    case T_TICKS: return "T_TICKS";
    case T_INSIDE: return "T_INSIDE";
    case T_OUTSIDE: return "T_OUTSIDE";
    case T_ROTATE: return "T_ROTATE";
    case T_LEGEND: return "T_LEGEND";
    case T_OVER: return "T_OVER";
    case T_TIMEWINDOW: return "T_TIMEWINDOW";
    default: return "T_UNKNOWN_TOKEN";
  }
}

Token::kTokenType Token::getType() const {
  return type_;
}

bool Token::operator==(const std::string& string) const {
  if (str_.size() != string.size()) {
    return false;
  }

  return strncasecmp(str_.c_str(), string.c_str(), string.size()) == 0;
}

bool Token::operator==(kTokenType type) const {
  return type_ == type;
}

bool Token::operator!=(kTokenType type) const {
  return type_ != type;
}

bool Token::operator==(const Token& other) const {
  if (type_ != other.type_) {
    return false;
  }

  return str_ == other.str_;
}

int64_t Token::getInteger() const {
  return std::stoi(str_); // FIXPAUL catch errors
}

const std::string Token::getString() const {
  return str_;
}

} // namespace csql

template <>
std::string inspect<
    csql::Token::kTokenType>(
    const csql::Token::kTokenType& value) {
  return csql::Token::getTypeName(value);
}

template <>
std::string inspect<
    csql::Token>(
    const csql::Token& value) {
  return csql::Token::getTypeName(value.getType());
}
