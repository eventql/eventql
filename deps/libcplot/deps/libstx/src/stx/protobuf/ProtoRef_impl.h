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

namespace stx {

template <class ProtoType>
ProtoRef<ProtoType>::ProtoRef() {}

template <class ProtoType>
ProtoRef<ProtoType>::ProtoRef(const ProtoType& _proto) :
  proto(_proto) {}

template <class ProtoType>
ProtoRef<ProtoType>::ProtoRef(
    const ProtoRef<ProtoType>& other) :
    proto(other.proto) {}

} // namespace stx

