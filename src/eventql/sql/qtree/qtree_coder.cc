/**
 * This file is part of the "libcsql" project
 *   Copyright (c) 2015 Paul Asmuth, zScale Technology GmbH
 *
 * libcsql is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <eventql/sql/qtree/qtree_coder.h>

using namespace stx;

namespace csql {

struct QueryTreeCoderTypeFactory {
  uint64_t wire_type;
  QueryTreeCoder::EncodeFn encode_fn;
  QueryTreeCoder::DecodeFn decode_fn;
};

static HashMap<const std::type_info*, QueryTreeCoderTypeFactory> coders_by_type_id{};
static HashMap<uint64_t, QueryTreeCoderTypeFactory> coders_by_wire_type_id{};

void QueryTreeCoder::encode(RefPtr<QueryTreeNode> tree, stx::OutputStream* os) {
  auto coder = coders_by_type_id.find(&typeid(*tree));
  if (coder == coders_by_type_id.end()) {
    RAISE(kIOError, "don't know how to encode this QueryTreeNode");
  }

  os->appendVarUInt(coder->second.wire_type);
  coder->second.encode_fn(tree, os);
}

RefPtr<QueryTreeNode> QueryTreeCoder::decode(Transaction* txn, stx::InputStream* is) {
  auto wire_type = is->readVarUInt();

  auto coder = coders_by_wire_type_id.find(wire_type);
  if (coder == coders_by_wire_type_id.end()) {
    RAISE(kIOError, "don't know how to decode this QueryTreeNode");
  }

  return coder->second.decode_fn(txn, is);
}

void QueryTreeCoder::registerType(
    const std::type_info* type_id,
    uint64_t wire_type_id,
    EncodeFn encode_fn,
    DecodeFn decode_fn) {
  QueryTreeCoderTypeFactory f;
  f.encode_fn = encode_fn;
  f.decode_fn = decode_fn;
  f.wire_type = wire_type_id;
  coders_by_type_id[type_id] = f;
  coders_by_wire_type_id[wire_type_id] = f;
}

} // namespace csql

