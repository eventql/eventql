/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_DOCUMENT_H
#define _CM_DOCUMENT_H
#include <mutex>
#include <stdlib.h>
#include <set>
#include <string>
#include <unordered_map>
#include <stx/autoref.h>
#include "DocID.h"
#include "IndexChangeRequest.h"

using namespace stx;

namespace zbase {

class Document : public RefCounted {
public:

  Document(const DocID& id);

  const DocID& docID() const;
  const HashMap<String, String>& fields() const;

  void setField(const String& field, const String& value);

  void debugPrint() const;

protected:
  HashMap<String, String> fields_;
  DocID id_;
};

} // namespace zbase

#endif
