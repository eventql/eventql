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

#ifndef _FNORDMETRIC_QUERY_TOKEN_H
#define _FNORDMETRIC_QUERY_TOKEN_H
#include <stdlib.h>
#include <string>
#include <vector>

namespace csql {

class Token {
public:
  enum kTokenType {
    T_SELECT,
    T_FROM,
    T_WHERE,
    T_GROUP,
    T_ORDER,
    T_BY,
    T_HAVING,
    T_LIMIT,
    T_OFFSET,
    T_ASC,
    T_DESC,
    T_COMMA,
    T_DOT,
    T_IDENTIFIER,
    T_STRING,
    T_NUMERIC,
    T_NULL,
    T_SEMICOLON,
    T_LPAREN,
    T_RPAREN,
    T_AND,
    T_OR,
    T_EQUAL,
    T_NEQUAL,
    T_PLUS,
    T_MINUS,
    T_ASTERISK,
    T_SLASH,
    T_AS,
    T_NOT,
    T_TRUE,
    T_FALSE,
    T_BANG,
    T_CIRCUMFLEX,
    T_TILDE,
    T_PERCENT,
    T_DIV,
    T_MOD,
    T_AMPERSAND,
    T_PIPE,
    T_LSHIFT,
    T_RSHIFT,
    T_LT,
    T_LTE,
    T_GT,
    T_GTE,
    T_LIKE,
    T_REGEX,
    T_BEGIN,
    T_CREATE,
    T_WITH,
    T_IMPORT,
    T_TABLE,
    T_TABLES,
    T_DATABASE,
    T_USE,
    T_EOF,
    T_SHOW,
    T_DESCRIBE,
    T_EXPLAIN,
    T_PARTITIONS,
    T_SERVERS,
    T_PRIMARY,
    T_KEY,
    T_REPEATED,
    T_INSERT,
    T_INTO,
    T_VALUES,
    T_JSON,
    T_ALTER,
    T_ADD,
    T_DROP,
    T_COLUMN,

    T_JOIN,
    T_CROSS,
    T_NATURAL,
    T_LEFT,
    T_RIGHT,
    T_INNER,
    T_OUTER,
    T_USING,
    T_ON,

    T_WITHIN,
    T_RECORD,

    T_OFF,
    T_DRAW,
    T_LINECHART,
    T_AREACHART,
    T_BARCHART,
    T_POINTCHART,
    T_HEATMAP,
    T_HISTOGRAM,
    T_AXIS,
    T_TOP,
    T_BOTTOM,
    T_ORIENTATION,
    T_HORIZONTAL,
    T_VERTICAL,
    T_STACKED,
    T_XDOMAIN,
    T_YDOMAIN,
    T_ZDOMAIN,
    T_XGRID,
    T_YGRID,
    T_LOGARITHMIC,
    T_INVERT,
    T_TITLE,
    T_SUBTITLE,
    T_GRID,
    T_LABELS,
    T_TICKS,
    T_INSIDE,
    T_OUTSIDE,
    T_ROTATE,
    T_LEGEND,
    T_OVER,
    T_TIMEWINDOW
  };

  Token(kTokenType token_type);
  Token(kTokenType token_type, const char* data, size_t size);
  Token(kTokenType token_type, const std::string& str);
  Token(const Token& copy);
  Token& operator=(const Token& copy) = delete;
  bool operator==(const Token& other) const;
  bool operator==(const std::string& string) const;
  bool operator==(kTokenType type) const;
  bool operator!=(kTokenType type) const;
  kTokenType getType() const;
  static const char* getTypeName(kTokenType type);
  const std::string getString() const;
  int64_t getInteger() const; // FIXPAUL removeme

  void debugPrint() const;

protected:
  const kTokenType type_;
  std::string str_;
};

}
#endif
