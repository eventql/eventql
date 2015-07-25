// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <cortex-flow/ir/ConstantValue.h>

namespace cortex {
namespace flow {

template class CORTEX_EXPORT ConstantValue<int64_t, FlowType::Number>;
template class CORTEX_EXPORT ConstantValue<bool, FlowType::Boolean>;
template class CORTEX_EXPORT ConstantValue<std::string, FlowType::String>;
template class CORTEX_EXPORT ConstantValue<IPAddress, FlowType::IPAddress>;
template class CORTEX_EXPORT ConstantValue<Cidr, FlowType::Cidr>;
template class CORTEX_EXPORT ConstantValue<RegExp, FlowType::RegExp>;

}  // namespace flow
}  // namespace cortex
