/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_PROTOCRDT_H
#define _CM_PROTOCRDT_H
#include <eventql/util/util/binarymessagereader.h>
#include <eventql/util/util/binarymessagewriter.h>

using namespace stx;

namespace zbase {

template <typename ProtoType>
struct ProtoCRDT {
  virtual ~ProtoCRDT() {};

  void decode(util::BinaryMessageReader* reader);
  void encode(util::BinaryMessageWriter* writer) const;
  virtual void merge(const ProtoCRDT<ProtoType>& other) = 0;

  ProtoType proto;
};


template <typename ProtoType>
void ProtoCRDT<ProtoType>::decode(
    util::BinaryMessageReader* reader) {
  auto size = reader->remaining();
  auto data = reader->read(size);
  msg::decode<ProtoType>(data, size, &proto);
}

template <typename ProtoType>
void ProtoCRDT<ProtoType>::encode(
    util::BinaryMessageWriter* writer) const {

}


} // namespace zbase

#endif
