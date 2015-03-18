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
#include <fnord-base/autoref.h>
#include "DocID.h"
#include "IndexRequest.h"

using namespace fnord;

namespace cm {

class Document : public RefCounted {
public:

  Document(const DocID& id);

  const DocID& docID() const;

  void update(const IndexRequest& index_req);

  void debugPrint() const;

protected:
  HashMap<String, String> fields_;
  DocID id_;
};

} // namespace cm

#endif
