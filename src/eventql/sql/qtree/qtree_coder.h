/**
 * This file is part of the "libcsql" project
 *   Copyright (c) 2015 Paul Asmuth, zScale Technology GmbH
 *
 * libcsql is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/sql/Transaction.h>

using namespace stx;

namespace csql {

class QueryTreeCoder {
public:

  template <class T>
  friend class QueryTreeCoderType;

  typedef Function<void (RefPtr<QueryTreeNode>, stx::OutputStream*)> EncodeFn;
  typedef Function<RefPtr<QueryTreeNode> (stx::InputStream*)> DecodeFn;

  static void encode(RefPtr<QueryTreeNode> tree, stx::OutputStream* os);
  static RefPtr<QueryTreeNode> decode(stx::InputStream* is);

protected:

  static void registerType(
      const std::type_info* type_id,
      uint64_t wire_type_id,
      EncodeFn encode_fn,
      DecodeFn decode_fn);

};

template <class T>
class QueryTreeCoderType {
public:
  QueryTreeCoderType();
};

} // namespace csql

#include "qtree_coder_impl.h"
