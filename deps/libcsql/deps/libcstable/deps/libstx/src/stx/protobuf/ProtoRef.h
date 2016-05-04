/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include "stx/stdtypes.h"

namespace stx {

template <class ProtoType>
struct ProtoRef : public Serializable, public RefCounted {
  ProtoRef();
  ProtoRef(const ProtoType& proto);
  ProtoRef(const ProtoRef<ProtoType>& other);
  ProtoType proto;
};

} // namespace stx

#include "ProtoRef_impl.h"
