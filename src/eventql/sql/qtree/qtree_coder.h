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

  QueryTreeCoder(Transaction* txn);

  template <class T>
  void registerType(uint64_t wire_type_id);

  void encode(RefPtr<QueryTreeNode> tree, stx::OutputStream* os);
  RefPtr<QueryTreeNode> decode(stx::InputStream* is);

protected:

  typedef Function<void (QueryTreeCoder*, RefPtr<QueryTreeNode>, stx::OutputStream*)> EncodeFn;
  typedef Function<RefPtr<QueryTreeNode> (QueryTreeCoder*, stx::InputStream*)> DecodeFn;

  struct QueryTreeCoderType {
    const std::type_info* type_id;
    uint64_t wire_type_id;
    QueryTreeCoder::EncodeFn encode_fn;
    QueryTreeCoder::DecodeFn decode_fn;
  };

  void registerType(QueryTreeCoderType f);

  Transaction* txn_;
  HashMap<const std::type_info*, QueryTreeCoderType> coders_by_type_id_;
  HashMap<uint64_t, QueryTreeCoderType> coders_by_wire_type_id_;
};

} // namespace csql

#include "qtree_coder_impl.h"
