// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <utility>
#include <memory>
#include <cstdint>
#include <cortex-flow/Api.h>

namespace cortex {
namespace flow {

//! \addtogroup Flow
//@{

struct CORTEX_FLOW_API FlowToken {
  enum _ {
    Unknown,

    // literals
    Boolean,
    Number,
    String,
    RawString,
    RegExp,
    IP,
    Cidr,
    NamedParam,
    InterpolatedStringFragment,  // "hello ${" or "} world ${"
    InterpolatedStringEnd,       // "} end"

    // symbols
    Assign,
    OrAssign,
    AndAssign,
    PlusAssign,
    MinusAssign,
    MulAssign,
    DivAssign,
    Semicolon,
    Question,
    Colon,
    And,
    Or,
    Xor,
    Equal,
    UnEqual,
    Less,
    Greater,
    LessOrEqual,
    GreaterOrEqual,
    PrefixMatch,
    SuffixMatch,
    RegexMatch,
    In,
    HashRocket,
    Plus,
    Minus,
    Mul,
    Div,
    Mod,
    Shl,
    Shr,
    Comma,
    Pow,
    Not,
    BitNot,
    BitOr,
    BitAnd,
    BitXor,
    BrOpen,
    BrClose,
    RndOpen,
    RndClose,
    Begin,
    End,

    // keywords
    Var,
    Do,
    Handler,
    If,
    Then,
    Else,
    Unless,
    Match,
    On,
    Import,
    From,

    // data types
    VoidType,
    BoolType,
    NumberType,
    StringType,

    // misc
    Ident,
    Period,
    DblPeriod,
    Ellipsis,
    Comment,
    Eof,
    COUNT
  };

 public:
  FlowToken() : value_(Unknown) {}
  FlowToken(int value) : value_(value) {}
  FlowToken(_ value) : value_(static_cast<int>(value)) {}

  int value() const throw() { return value_; }
  const char *c_str() const throw();

  operator int() const { return value_; }

 private:
  int value_;
};

class CORTEX_FLOW_API FlowTokenTraits {
 public:
  static bool isKeyword(FlowToken t);
  static bool isReserved(FlowToken t);
  static bool isLiteral(FlowToken t);
  static bool isType(FlowToken t);

  static bool isOperator(FlowToken t);
  static bool isUnaryOp(FlowToken t);
  static bool isPrimaryOp(FlowToken t);
  static bool isRelOp(FlowToken t);
};

//!@}

}  // namespace flow
}  // namespace cortex

namespace std {

//! \addtogroup Flow
//@{

template <>
struct hash<cortex::flow::FlowToken> {
  uint32_t operator()(cortex::flow::FlowToken v) const {
    return static_cast<uint32_t>(v.value());
  }
};

//!@}
}
