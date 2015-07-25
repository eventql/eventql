// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <cortex-flow/Api.h>
#include <cortex-base/Buffer.h>
#include <string>
#include <vector>
#include <cstdint>

namespace cortex {

class IPAddress;
class Cidr;

namespace flow {

//! \addtogroup Flow
//@{

enum class FlowType {
  Void = 0,
  Boolean = 1,       // bool (int64)
  Number = 2,        // int64
  String = 3,        // BufferRef*
  IPAddress = 5,     // IPAddress*
  Cidr = 6,          // Cidr*
  RegExp = 7,        // RegExp*
  Handler = 8,       // bool (*native_handler)(FlowContext*);
  IntArray = 9,      // array<int>
  StringArray = 10,  // array<string>
  IPAddrArray = 11,  // array<IPAddress>
  CidrArray = 12,    // array<Cidr>
};

typedef uint64_t Register;  // vm

typedef int64_t FlowNumber;
typedef BufferRef FlowString;

std::string tos(FlowType type);

// {{{ array types
class CORTEX_FLOW_API FlowArray {
 public:
  size_t size() const { return base_[0]; }

 protected:
  explicit FlowArray(const Register* base) : base_(base) {}

  Register getRawAt(size_t i) const { return base_[1 + i]; }
  const Register* data() const { return base_ + 1; }

 protected:
  const Register* base_;
};

typedef std::vector<FlowNumber> FlowIntArray;
typedef std::vector<FlowString> FlowStringArray;
typedef std::vector<IPAddress> FlowIPAddrArray;
typedef std::vector<Cidr> FlowCidrArray;

// }}}

//!@}

}  // namespace flow
}  // namespace cortex

namespace std {

template <>
struct hash<cortex::flow::FlowType> {
  uint32_t operator()(cortex::flow::FlowType v) const noexcept {
    return static_cast<uint32_t>(v);
  }
};

}
