/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <eventql/util/inspect.h>
#include <eventql/docdb/ItemRef.h>

namespace zbase {

bool ItemRef::operator==(const ItemRef& other) const {
  return set_id == other.set_id && item_id == other.item_id;
}

DocID ItemRef::docID() const {
  return DocID{ .docid = set_id + "~" + item_id };
}

}

namespace stx {

template <>
std::string inspect(const zbase::ItemRef& itemref) {
  return StringUtil::format("$0~$1", itemref.set_id, itemref.item_id);
};

template <>
String inspect(const zbase::DocID& docid) {
  return docid.docid;
}

}
